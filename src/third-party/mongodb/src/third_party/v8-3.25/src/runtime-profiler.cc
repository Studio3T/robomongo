// Copyright 2012 the V8 project authors. All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//     * Neither the name of Google Inc. nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "v8.h"

#include "runtime-profiler.h"

#include "assembler.h"
#include "bootstrapper.h"
#include "code-stubs.h"
#include "compilation-cache.h"
#include "execution.h"
#include "full-codegen.h"
#include "global-handles.h"
#include "isolate-inl.h"
#include "mark-compact.h"
#include "platform.h"
#include "scopeinfo.h"

namespace v8 {
namespace internal {


// Number of times a function has to be seen on the stack before it is
// optimized.
static const int kProfilerTicksBeforeOptimization = 2;
// If the function optimization was disabled due to high deoptimization count,
// but the function is hot and has been seen on the stack this number of times,
// then we try to reenable optimization for this function.
static const int kProfilerTicksBeforeReenablingOptimization = 250;
// If a function does not have enough type info (according to
// FLAG_type_info_threshold), but has seen a huge number of ticks,
// optimize it as it is.
static const int kTicksWhenNotEnoughTypeInfo = 100;
// We only have one byte to store the number of ticks.
STATIC_ASSERT(kProfilerTicksBeforeOptimization < 256);
STATIC_ASSERT(kProfilerTicksBeforeReenablingOptimization < 256);
STATIC_ASSERT(kTicksWhenNotEnoughTypeInfo < 256);

// Maximum size in bytes of generate code for a function to allow OSR.
static const int kOSRCodeSizeAllowanceBase =
    100 * FullCodeGenerator::kCodeSizeMultiplier;

static const int kOSRCodeSizeAllowancePerTick =
    4 * FullCodeGenerator::kCodeSizeMultiplier;

// Maximum size in bytes of generated code for a function to be optimized
// the very first time it is seen on the stack.
static const int kMaxSizeEarlyOpt =
    5 * FullCodeGenerator::kCodeSizeMultiplier;


RuntimeProfiler::RuntimeProfiler(Isolate* isolate)
    : isolate_(isolate),
      any_ic_changed_(false) {
}


static void GetICCounts(Code* shared_code,
                        int* ic_with_type_info_count,
                        int* ic_total_count,
                        int* percentage) {
  *ic_total_count = 0;
  *ic_with_type_info_count = 0;
  Object* raw_info = shared_code->type_feedback_info();
  if (raw_info->IsTypeFeedbackInfo()) {
    TypeFeedbackInfo* info = TypeFeedbackInfo::cast(raw_info);
    *ic_with_type_info_count = info->ic_with_type_info_count();
    *ic_total_count = info->ic_total_count();
  }
  *percentage = *ic_total_count > 0
      ? 100 * *ic_with_type_info_count / *ic_total_count
      : 100;
}


void RuntimeProfiler::Optimize(JSFunction* function, const char* reason) {
  ASSERT(function->IsOptimizable());

  if (FLAG_trace_opt && function->PassesFilter(FLAG_hydrogen_filter)) {
    PrintF("[marking ");
    function->ShortPrint();
    PrintF(" for recompilation, reason: %s", reason);
    if (FLAG_type_info_threshold > 0) {
      int typeinfo, total, percentage;
      GetICCounts(function->shared()->code(), &typeinfo, &total, &percentage);
      PrintF(", ICs with typeinfo: %d/%d (%d%%)", typeinfo, total, percentage);
    }
    PrintF("]\n");
  }


  if (isolate_->concurrent_recompilation_enabled() &&
      !isolate_->bootstrapper()->IsActive()) {
    if (isolate_->concurrent_osr_enabled() &&
        isolate_->optimizing_compiler_thread()->IsQueuedForOSR(function)) {
      // Do not attempt regular recompilation if we already queued this for OSR.
      // TODO(yangguo): This is necessary so that we don't install optimized
      // code on a function that is already optimized, since OSR and regular
      // recompilation race.  This goes away as soon as OSR becomes one-shot.
      return;
    }
    ASSERT(!function->IsInOptimizationQueue());
    function->MarkForConcurrentOptimization();
  } else {
    // The next call to the function will trigger optimization.
    function->MarkForOptimization();
  }
}


void RuntimeProfiler::AttemptOnStackReplacement(JSFunction* function) {
  // See AlwaysFullCompiler (in compiler.cc) comment on why we need
  // Debug::has_break_points().
  if (!FLAG_use_osr ||
      isolate_->DebuggerHasBreakPoints() ||
      function->IsBuiltin()) {
    return;
  }

  SharedFunctionInfo* shared = function->shared();
  // If the code is not optimizable, don't try OSR.
  if (!shared->code()->optimizable()) return;

  // We are not prepared to do OSR for a function that already has an
  // allocated arguments object.  The optimized code would bypass it for
  // arguments accesses, which is unsound.  Don't try OSR.
  if (shared->uses_arguments()) return;

  // We're using on-stack replacement: patch the unoptimized code so that
  // any back edge in any unoptimized frame will trigger on-stack
  // replacement for that frame.
  if (FLAG_trace_osr) {
    PrintF("[OSR - patching back edges in ");
    function->PrintName();
    PrintF("]\n");
  }

  BackEdgeTable::Patch(isolate_, shared->code());
}


void RuntimeProfiler::OptimizeNow() {
  HandleScope scope(isolate_);

  if (isolate_->DebuggerHasBreakPoints()) return;

  DisallowHeapAllocation no_gc;

  // Run through the JavaScript frames and collect them. If we already
  // have a sample of the function, we mark it for optimizations
  // (eagerly or lazily).
  int frame_count = 0;
  int frame_count_limit = FLAG_frame_count;
  for (JavaScriptFrameIterator it(isolate_);
       frame_count++ < frame_count_limit && !it.done();
       it.Advance()) {
    JavaScriptFrame* frame = it.frame();
    JSFunction* function = frame->function();

    SharedFunctionInfo* shared = function->shared();
    Code* shared_code = shared->code();

    if (shared_code->kind() != Code::FUNCTION) continue;
    if (function->IsInOptimizationQueue()) continue;

    if (FLAG_always_osr &&
        shared_code->allow_osr_at_loop_nesting_level() == 0) {
      // Testing mode: always try an OSR compile for every function.
      for (int i = 0; i < Code::kMaxLoopNestingMarker; i++) {
        // TODO(titzer): fix AttemptOnStackReplacement to avoid this dumb loop.
        shared_code->set_allow_osr_at_loop_nesting_level(i);
        AttemptOnStackReplacement(function);
      }
      // Fall through and do a normal optimized compile as well.
    } else if (!frame->is_optimized() &&
        (function->IsMarkedForOptimization() ||
         function->IsMarkedForConcurrentOptimization() ||
         function->IsOptimized())) {
      // Attempt OSR if we are still running unoptimized code even though the
      // the function has long been marked or even already been optimized.
      int ticks = shared_code->profiler_ticks();
      int allowance = kOSRCodeSizeAllowanceBase +
                      ticks * kOSRCodeSizeAllowancePerTick;
      if (shared_code->CodeSize() > allowance) {
        if (ticks < 255) shared_code->set_profiler_ticks(ticks + 1);
      } else {
        int nesting = shared_code->allow_osr_at_loop_nesting_level();
        if (nesting < Code::kMaxLoopNestingMarker) {
          int new_nesting = nesting + 1;
          shared_code->set_allow_osr_at_loop_nesting_level(new_nesting);
          AttemptOnStackReplacement(function);
        }
      }
      continue;
    }

    // Only record top-level code on top of the execution stack and
    // avoid optimizing excessively large scripts since top-level code
    // will be executed only once.
    const int kMaxToplevelSourceSize = 10 * 1024;
    if (shared->is_toplevel() &&
        (frame_count > 1 || shared->SourceSize() > kMaxToplevelSourceSize)) {
      continue;
    }

    // Do not record non-optimizable functions.
    if (shared->optimization_disabled()) {
      if (shared->deopt_count() >= FLAG_max_opt_count) {
        // If optimization was disabled due to many deoptimizations,
        // then check if the function is hot and try to reenable optimization.
        int ticks = shared_code->profiler_ticks();
        if (ticks >= kProfilerTicksBeforeReenablingOptimization) {
          shared_code->set_profiler_ticks(0);
          shared->TryReenableOptimization();
        } else {
          shared_code->set_profiler_ticks(ticks + 1);
        }
      }
      continue;
    }
    if (!function->IsOptimizable()) continue;

    int ticks = shared_code->profiler_ticks();

    if (ticks >= kProfilerTicksBeforeOptimization) {
      int typeinfo, total, percentage;
      GetICCounts(shared_code, &typeinfo, &total, &percentage);
      if (percentage >= FLAG_type_info_threshold) {
        // If this particular function hasn't had any ICs patched for enough
        // ticks, optimize it now.
        Optimize(function, "hot and stable");
      } else if (ticks >= kTicksWhenNotEnoughTypeInfo) {
        Optimize(function, "not much type info but very hot");
      } else {
        shared_code->set_profiler_ticks(ticks + 1);
        if (FLAG_trace_opt_verbose) {
          PrintF("[not yet optimizing ");
          function->PrintName();
          PrintF(", not enough type info: %d/%d (%d%%)]\n",
                 typeinfo, total, percentage);
        }
      }
    } else if (!any_ic_changed_ &&
               shared_code->instruction_size() < kMaxSizeEarlyOpt) {
      // If no IC was patched since the last tick and this function is very
      // small, optimistically optimize it now.
      Optimize(function, "small function");
    } else {
      shared_code->set_profiler_ticks(ticks + 1);
    }
  }
  any_ic_changed_ = false;
}


} }  // namespace v8::internal
