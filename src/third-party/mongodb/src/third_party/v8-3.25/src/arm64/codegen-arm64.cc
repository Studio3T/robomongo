// Copyright 2013 the V8 project authors. All rights reserved.
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

#if V8_TARGET_ARCH_ARM64

#include "codegen.h"
#include "macro-assembler.h"
#include "simulator-arm64.h"

namespace v8 {
namespace internal {

#define __ ACCESS_MASM(masm)

#if defined(USE_SIMULATOR)
byte* fast_exp_arm64_machine_code = NULL;
double fast_exp_simulator(double x) {
  Simulator * simulator = Simulator::current(Isolate::Current());
  Simulator::CallArgument args[] = {
      Simulator::CallArgument(x),
      Simulator::CallArgument::End()
  };
  return simulator->CallDouble(fast_exp_arm64_machine_code, args);
}
#endif


UnaryMathFunction CreateExpFunction() {
  if (!FLAG_fast_math) return &std::exp;

  // Use the Math.exp implemetation in MathExpGenerator::EmitMathExp() to create
  // an AAPCS64-compliant exp() function. This will be faster than the C
  // library's exp() function, but probably less accurate.
  size_t actual_size;
  byte* buffer = static_cast<byte*>(OS::Allocate(1 * KB, &actual_size, true));
  if (buffer == NULL) return &std::exp;

  ExternalReference::InitializeMathExpData();
  MacroAssembler masm(NULL, buffer, static_cast<int>(actual_size));
  masm.SetStackPointer(csp);

  // The argument will be in d0 on entry.
  DoubleRegister input = d0;
  // Use other caller-saved registers for all other values.
  DoubleRegister result = d1;
  DoubleRegister double_temp1 = d2;
  DoubleRegister double_temp2 = d3;
  Register temp1 = x10;
  Register temp2 = x11;
  Register temp3 = x12;

  MathExpGenerator::EmitMathExp(&masm, input, result,
                                double_temp1, double_temp2,
                                temp1, temp2, temp3);
  // Move the result to the return register.
  masm.Fmov(d0, result);
  masm.Ret();

  CodeDesc desc;
  masm.GetCode(&desc);
  ASSERT(!RelocInfo::RequiresRelocation(desc));

  CPU::FlushICache(buffer, actual_size);
  OS::ProtectCode(buffer, actual_size);

#if !defined(USE_SIMULATOR)
  return FUNCTION_CAST<UnaryMathFunction>(buffer);
#else
  fast_exp_arm64_machine_code = buffer;
  return &fast_exp_simulator;
#endif
}


UnaryMathFunction CreateSqrtFunction() {
  return &std::sqrt;
}


// -------------------------------------------------------------------------
// Platform-specific RuntimeCallHelper functions.

void StubRuntimeCallHelper::BeforeCall(MacroAssembler* masm) const {
  masm->EnterFrame(StackFrame::INTERNAL);
  ASSERT(!masm->has_frame());
  masm->set_has_frame(true);
}


void StubRuntimeCallHelper::AfterCall(MacroAssembler* masm) const {
  masm->LeaveFrame(StackFrame::INTERNAL);
  ASSERT(masm->has_frame());
  masm->set_has_frame(false);
}


// -------------------------------------------------------------------------
// Code generators

void ElementsTransitionGenerator::GenerateMapChangeElementsTransition(
    MacroAssembler* masm, AllocationSiteMode mode,
    Label* allocation_memento_found) {
  // ----------- S t a t e -------------
  //  -- x2    : receiver
  //  -- x3    : target map
  // -----------------------------------
  Register receiver = x2;
  Register map = x3;

  if (mode == TRACK_ALLOCATION_SITE) {
    ASSERT(allocation_memento_found != NULL);
    __ JumpIfJSArrayHasAllocationMemento(receiver, x10, x11,
                                         allocation_memento_found);
  }

  // Set transitioned map.
  __ Str(map, FieldMemOperand(receiver, HeapObject::kMapOffset));
  __ RecordWriteField(receiver,
                      HeapObject::kMapOffset,
                      map,
                      x10,
                      kLRHasNotBeenSaved,
                      kDontSaveFPRegs,
                      EMIT_REMEMBERED_SET,
                      OMIT_SMI_CHECK);
}


void ElementsTransitionGenerator::GenerateSmiToDouble(
    MacroAssembler* masm, AllocationSiteMode mode, Label* fail) {
  ASM_LOCATION("ElementsTransitionGenerator::GenerateSmiToDouble");
  // ----------- S t a t e -------------
  //  -- lr    : return address
  //  -- x0    : value
  //  -- x1    : key
  //  -- x2    : receiver
  //  -- x3    : target map, scratch for subsequent call
  // -----------------------------------
  Register receiver = x2;
  Register target_map = x3;

  Label gc_required, only_change_map;

  if (mode == TRACK_ALLOCATION_SITE) {
    __ JumpIfJSArrayHasAllocationMemento(receiver, x10, x11, fail);
  }

  // Check for empty arrays, which only require a map transition and no changes
  // to the backing store.
  Register elements = x4;
  __ Ldr(elements, FieldMemOperand(receiver, JSObject::kElementsOffset));
  __ JumpIfRoot(elements, Heap::kEmptyFixedArrayRootIndex, &only_change_map);

  __ Push(lr);
  Register length = x5;
  __ Ldrsw(length, UntagSmiFieldMemOperand(elements,
                                           FixedArray::kLengthOffset));

  // Allocate new FixedDoubleArray.
  Register array_size = x6;
  Register array = x7;
  __ Lsl(array_size, length, kDoubleSizeLog2);
  __ Add(array_size, array_size, FixedDoubleArray::kHeaderSize);
  __ Allocate(array_size, array, x10, x11, &gc_required, DOUBLE_ALIGNMENT);
  // Register array is non-tagged heap object.

  // Set the destination FixedDoubleArray's length and map.
  Register map_root = x6;
  __ LoadRoot(map_root, Heap::kFixedDoubleArrayMapRootIndex);
  __ SmiTag(x11, length);
  __ Str(x11, MemOperand(array, FixedDoubleArray::kLengthOffset));
  __ Str(map_root, MemOperand(array, HeapObject::kMapOffset));

  __ Str(target_map, FieldMemOperand(receiver, HeapObject::kMapOffset));
  __ RecordWriteField(receiver, HeapObject::kMapOffset, target_map, x6,
                      kLRHasBeenSaved, kDontSaveFPRegs, OMIT_REMEMBERED_SET,
                      OMIT_SMI_CHECK);

  // Replace receiver's backing store with newly created FixedDoubleArray.
  __ Add(x10, array, kHeapObjectTag);
  __ Str(x10, FieldMemOperand(receiver, JSObject::kElementsOffset));
  __ RecordWriteField(receiver, JSObject::kElementsOffset, x10,
                      x6, kLRHasBeenSaved, kDontSaveFPRegs,
                      EMIT_REMEMBERED_SET, OMIT_SMI_CHECK);

  // Prepare for conversion loop.
  Register src_elements = x10;
  Register dst_elements = x11;
  Register dst_end = x12;
  __ Add(src_elements, elements, FixedArray::kHeaderSize - kHeapObjectTag);
  __ Add(dst_elements, array, FixedDoubleArray::kHeaderSize);
  __ Add(dst_end, dst_elements, Operand(length, LSL, kDoubleSizeLog2));

  FPRegister nan_d = d1;
  __ Fmov(nan_d, rawbits_to_double(kHoleNanInt64));

  Label entry, done;
  __ B(&entry);

  __ Bind(&only_change_map);
  __ Str(target_map, FieldMemOperand(receiver, HeapObject::kMapOffset));
  __ RecordWriteField(receiver, HeapObject::kMapOffset, target_map, x6,
                      kLRHasNotBeenSaved, kDontSaveFPRegs, OMIT_REMEMBERED_SET,
                      OMIT_SMI_CHECK);
  __ B(&done);

  // Call into runtime if GC is required.
  __ Bind(&gc_required);
  __ Pop(lr);
  __ B(fail);

  // Iterate over the array, copying and coverting smis to doubles. If an
  // element is non-smi, write a hole to the destination.
  {
    Label loop;
    __ Bind(&loop);
    __ Ldr(x13, MemOperand(src_elements, kPointerSize, PostIndex));
    __ SmiUntagToDouble(d0, x13, kSpeculativeUntag);
    __ Tst(x13, kSmiTagMask);
    __ Fcsel(d0, d0, nan_d, eq);
    __ Str(d0, MemOperand(dst_elements, kDoubleSize, PostIndex));

    __ Bind(&entry);
    __ Cmp(dst_elements, dst_end);
    __ B(lt, &loop);
  }

  __ Pop(lr);
  __ Bind(&done);
}


void ElementsTransitionGenerator::GenerateDoubleToObject(
    MacroAssembler* masm, AllocationSiteMode mode, Label* fail) {
  ASM_LOCATION("ElementsTransitionGenerator::GenerateDoubleToObject");
  // ----------- S t a t e -------------
  //  -- x0    : value
  //  -- x1    : key
  //  -- x2    : receiver
  //  -- lr    : return address
  //  -- x3    : target map, scratch for subsequent call
  //  -- x4    : scratch (elements)
  // -----------------------------------
  Register value = x0;
  Register key = x1;
  Register receiver = x2;
  Register target_map = x3;

  if (mode == TRACK_ALLOCATION_SITE) {
    __ JumpIfJSArrayHasAllocationMemento(receiver, x10, x11, fail);
  }

  // Check for empty arrays, which only require a map transition and no changes
  // to the backing store.
  Label only_change_map;
  Register elements = x4;
  __ Ldr(elements, FieldMemOperand(receiver, JSObject::kElementsOffset));
  __ JumpIfRoot(elements, Heap::kEmptyFixedArrayRootIndex, &only_change_map);

  __ Push(lr);
  // TODO(all): These registers may not need to be pushed. Examine
  // RecordWriteStub and check whether it's needed.
  __ Push(target_map, receiver, key, value);
  Register length = x5;
  __ Ldrsw(length, UntagSmiFieldMemOperand(elements,
                                           FixedArray::kLengthOffset));

  // Allocate new FixedArray.
  Register array_size = x6;
  Register array = x7;
  Label gc_required;
  __ Mov(array_size, FixedDoubleArray::kHeaderSize);
  __ Add(array_size, array_size, Operand(length, LSL, kPointerSizeLog2));
  __ Allocate(array_size, array, x10, x11, &gc_required, NO_ALLOCATION_FLAGS);

  // Set destination FixedDoubleArray's length and map.
  Register map_root = x6;
  __ LoadRoot(map_root, Heap::kFixedArrayMapRootIndex);
  __ SmiTag(x11, length);
  __ Str(x11, MemOperand(array, FixedDoubleArray::kLengthOffset));
  __ Str(map_root, MemOperand(array, HeapObject::kMapOffset));

  // Prepare for conversion loop.
  Register src_elements = x10;
  Register dst_elements = x11;
  Register dst_end = x12;
  __ Add(src_elements, elements,
         FixedDoubleArray::kHeaderSize - kHeapObjectTag);
  __ Add(dst_elements, array, FixedArray::kHeaderSize);
  __ Add(array, array, kHeapObjectTag);
  __ Add(dst_end, dst_elements, Operand(length, LSL, kPointerSizeLog2));

  Register the_hole = x14;
  Register heap_num_map = x15;
  __ LoadRoot(the_hole, Heap::kTheHoleValueRootIndex);
  __ LoadRoot(heap_num_map, Heap::kHeapNumberMapRootIndex);

  Label entry;
  __ B(&entry);

  // Call into runtime if GC is required.
  __ Bind(&gc_required);
  __ Pop(value, key, receiver, target_map);
  __ Pop(lr);
  __ B(fail);

  {
    Label loop, convert_hole;
    __ Bind(&loop);
    __ Ldr(x13, MemOperand(src_elements, kPointerSize, PostIndex));
    __ Cmp(x13, kHoleNanInt64);
    __ B(eq, &convert_hole);

    // Non-hole double, copy value into a heap number.
    Register heap_num = x5;
    __ AllocateHeapNumber(heap_num, &gc_required, x6, x4, heap_num_map);
    __ Str(x13, FieldMemOperand(heap_num, HeapNumber::kValueOffset));
    __ Mov(x13, dst_elements);
    __ Str(heap_num, MemOperand(dst_elements, kPointerSize, PostIndex));
    __ RecordWrite(array, x13, heap_num, kLRHasBeenSaved, kDontSaveFPRegs,
                   EMIT_REMEMBERED_SET, OMIT_SMI_CHECK);

    __ B(&entry);

    // Replace the-hole NaN with the-hole pointer.
    __ Bind(&convert_hole);
    __ Str(the_hole, MemOperand(dst_elements, kPointerSize, PostIndex));

    __ Bind(&entry);
    __ Cmp(dst_elements, dst_end);
    __ B(lt, &loop);
  }

  __ Pop(value, key, receiver, target_map);
  // Replace receiver's backing store with newly created and filled FixedArray.
  __ Str(array, FieldMemOperand(receiver, JSObject::kElementsOffset));
  __ RecordWriteField(receiver, JSObject::kElementsOffset, array, x13,
                      kLRHasBeenSaved, kDontSaveFPRegs, EMIT_REMEMBERED_SET,
                      OMIT_SMI_CHECK);
  __ Pop(lr);

  __ Bind(&only_change_map);
  __ Str(target_map, FieldMemOperand(receiver, HeapObject::kMapOffset));
  __ RecordWriteField(receiver, HeapObject::kMapOffset, target_map, x13,
                      kLRHasNotBeenSaved, kDontSaveFPRegs, OMIT_REMEMBERED_SET,
                      OMIT_SMI_CHECK);
}


bool Code::IsYoungSequence(byte* sequence) {
  return MacroAssembler::IsYoungSequence(sequence);
}


void Code::GetCodeAgeAndParity(byte* sequence, Age* age,
                               MarkingParity* parity) {
  if (IsYoungSequence(sequence)) {
    *age = kNoAgeCodeAge;
    *parity = NO_MARKING_PARITY;
  } else {
    byte* target = sequence + kCodeAgeStubEntryOffset;
    Code* stub = GetCodeFromTargetAddress(Memory::Address_at(target));
    GetCodeAgeAndParity(stub, age, parity);
  }
}


void Code::PatchPlatformCodeAge(Isolate* isolate,
                                byte* sequence,
                                Code::Age age,
                                MarkingParity parity) {
  PatchingAssembler patcher(sequence, kCodeAgeSequenceSize / kInstructionSize);
  if (age == kNoAgeCodeAge) {
    MacroAssembler::EmitFrameSetupForCodeAgePatching(&patcher);
  } else {
    Code * stub = GetCodeAgeStub(isolate, age, parity);
    MacroAssembler::EmitCodeAgeSequence(&patcher, stub);
  }
}


void StringCharLoadGenerator::Generate(MacroAssembler* masm,
                                       Register string,
                                       Register index,
                                       Register result,
                                       Label* call_runtime) {
  ASSERT(string.Is64Bits() && index.Is32Bits() && result.Is64Bits());
  // Fetch the instance type of the receiver into result register.
  __ Ldr(result, FieldMemOperand(string, HeapObject::kMapOffset));
  __ Ldrb(result, FieldMemOperand(result, Map::kInstanceTypeOffset));

  // We need special handling for indirect strings.
  Label check_sequential;
  __ TestAndBranchIfAllClear(result, kIsIndirectStringMask, &check_sequential);

  // Dispatch on the indirect string shape: slice or cons.
  Label cons_string;
  __ TestAndBranchIfAllClear(result, kSlicedNotConsMask, &cons_string);

  // Handle slices.
  Label indirect_string_loaded;
  __ Ldr(result.W(),
         UntagSmiFieldMemOperand(string, SlicedString::kOffsetOffset));
  __ Ldr(string, FieldMemOperand(string, SlicedString::kParentOffset));
  __ Add(index, index, result.W());
  __ B(&indirect_string_loaded);

  // Handle cons strings.
  // Check whether the right hand side is the empty string (i.e. if
  // this is really a flat string in a cons string). If that is not
  // the case we would rather go to the runtime system now to flatten
  // the string.
  __ Bind(&cons_string);
  __ Ldr(result, FieldMemOperand(string, ConsString::kSecondOffset));
  __ JumpIfNotRoot(result, Heap::kempty_stringRootIndex, call_runtime);
  // Get the first of the two strings and load its instance type.
  __ Ldr(string, FieldMemOperand(string, ConsString::kFirstOffset));

  __ Bind(&indirect_string_loaded);
  __ Ldr(result, FieldMemOperand(string, HeapObject::kMapOffset));
  __ Ldrb(result, FieldMemOperand(result, Map::kInstanceTypeOffset));

  // Distinguish sequential and external strings. Only these two string
  // representations can reach here (slices and flat cons strings have been
  // reduced to the underlying sequential or external string).
  Label external_string, check_encoding;
  __ Bind(&check_sequential);
  STATIC_ASSERT(kSeqStringTag == 0);
  __ TestAndBranchIfAnySet(result, kStringRepresentationMask, &external_string);

  // Prepare sequential strings
  STATIC_ASSERT(SeqTwoByteString::kHeaderSize == SeqOneByteString::kHeaderSize);
  __ Add(string, string, SeqTwoByteString::kHeaderSize - kHeapObjectTag);
  __ B(&check_encoding);

  // Handle external strings.
  __ Bind(&external_string);
  if (FLAG_debug_code) {
    // Assert that we do not have a cons or slice (indirect strings) here.
    // Sequential strings have already been ruled out.
    __ Tst(result, kIsIndirectStringMask);
    __ Assert(eq, kExternalStringExpectedButNotFound);
  }
  // Rule out short external strings.
  STATIC_CHECK(kShortExternalStringTag != 0);
  // TestAndBranchIfAnySet can emit Tbnz. Do not use it because call_runtime
  // can be bound far away in deferred code.
  __ Tst(result, kShortExternalStringMask);
  __ B(ne, call_runtime);
  __ Ldr(string, FieldMemOperand(string, ExternalString::kResourceDataOffset));

  Label ascii, done;
  __ Bind(&check_encoding);
  STATIC_ASSERT(kTwoByteStringTag == 0);
  __ TestAndBranchIfAnySet(result, kStringEncodingMask, &ascii);
  // Two-byte string.
  __ Ldrh(result, MemOperand(string, index, SXTW, 1));
  __ B(&done);
  __ Bind(&ascii);
  // Ascii string.
  __ Ldrb(result, MemOperand(string, index, SXTW));
  __ Bind(&done);
}


static MemOperand ExpConstant(Register base, int index) {
  return MemOperand(base, index * kDoubleSize);
}


void MathExpGenerator::EmitMathExp(MacroAssembler* masm,
                                   DoubleRegister input,
                                   DoubleRegister result,
                                   DoubleRegister double_temp1,
                                   DoubleRegister double_temp2,
                                   Register temp1,
                                   Register temp2,
                                   Register temp3) {
  // TODO(jbramley): There are several instances where fnmsub could be used
  // instead of fmul and fsub. Doing this changes the result, but since this is
  // an estimation anyway, does it matter?

  ASSERT(!AreAliased(input, result,
                     double_temp1, double_temp2,
                     temp1, temp2, temp3));
  ASSERT(ExternalReference::math_exp_constants(0).address() != NULL);

  Label done;
  DoubleRegister double_temp3 = result;
  Register constants = temp3;

  // The algorithm used relies on some magic constants which are initialized in
  // ExternalReference::InitializeMathExpData().

  // Load the address of the start of the array.
  __ Mov(constants, ExternalReference::math_exp_constants(0));

  // We have to do a four-way split here:
  //  - If input <= about -708.4, the output always rounds to zero.
  //  - If input >= about 709.8, the output always rounds to +infinity.
  //  - If the input is NaN, the output is NaN.
  //  - Otherwise, the result needs to be calculated.
  Label result_is_finite_non_zero;
  // Assert that we can load offset 0 (the small input threshold) and offset 1
  // (the large input threshold) with a single ldp.
  ASSERT(kDRegSize == (ExpConstant(constants, 1).offset() -
                              ExpConstant(constants, 0).offset()));
  __ Ldp(double_temp1, double_temp2, ExpConstant(constants, 0));

  __ Fcmp(input, double_temp1);
  __ Fccmp(input, double_temp2, NoFlag, hi);
  // At this point, the condition flags can be in one of five states:
  //  NZCV
  //  1000      -708.4 < input < 709.8    result = exp(input)
  //  0110      input == 709.8            result = +infinity
  //  0010      input > 709.8             result = +infinity
  //  0011      input is NaN              result = input
  //  0000      input <= -708.4           result = +0.0

  // Continue the common case first. 'mi' tests N == 1.
  __ B(&result_is_finite_non_zero, mi);

  // TODO(jbramley): Consider adding a +infinity register for ARM64.
  __ Ldr(double_temp2, ExpConstant(constants, 2));    // Synthesize +infinity.

  // Select between +0.0 and +infinity. 'lo' tests C == 0.
  __ Fcsel(result, fp_zero, double_temp2, lo);
  // Select between {+0.0 or +infinity} and input. 'vc' tests V == 0.
  __ Fcsel(result, result, input, vc);
  __ B(&done);

  // The rest is magic, as described in InitializeMathExpData().
  __ Bind(&result_is_finite_non_zero);

  // Assert that we can load offset 3 and offset 4 with a single ldp.
  ASSERT(kDRegSize == (ExpConstant(constants, 4).offset() -
                              ExpConstant(constants, 3).offset()));
  __ Ldp(double_temp1, double_temp3, ExpConstant(constants, 3));
  __ Fmadd(double_temp1, double_temp1, input, double_temp3);
  __ Fmov(temp2.W(), double_temp1.S());
  __ Fsub(double_temp1, double_temp1, double_temp3);

  // Assert that we can load offset 5 and offset 6 with a single ldp.
  ASSERT(kDRegSize == (ExpConstant(constants, 6).offset() -
                              ExpConstant(constants, 5).offset()));
  __ Ldp(double_temp2, double_temp3, ExpConstant(constants, 5));
  // TODO(jbramley): Consider using Fnmsub here.
  __ Fmul(double_temp1, double_temp1, double_temp2);
  __ Fsub(double_temp1, double_temp1, input);

  __ Fmul(double_temp2, double_temp1, double_temp1);
  __ Fsub(double_temp3, double_temp3, double_temp1);
  __ Fmul(double_temp3, double_temp3, double_temp2);

  __ Mov(temp1.W(), Operand(temp2.W(), LSR, 11));

  __ Ldr(double_temp2, ExpConstant(constants, 7));
  // TODO(jbramley): Consider using Fnmsub here.
  __ Fmul(double_temp3, double_temp3, double_temp2);
  __ Fsub(double_temp3, double_temp3, double_temp1);

  // The 8th constant is 1.0, so use an immediate move rather than a load.
  // We can't generate a runtime assertion here as we would need to call Abort
  // in the runtime and we don't have an Isolate when we generate this code.
  __ Fmov(double_temp2, 1.0);
  __ Fadd(double_temp3, double_temp3, double_temp2);

  __ And(temp2, temp2, 0x7ff);
  __ Add(temp1, temp1, 0x3ff);

  // Do the final table lookup.
  __ Mov(temp3, ExternalReference::math_exp_log_table());

  __ Add(temp3, temp3, Operand(temp2, LSL, kDRegSizeLog2));
  __ Ldp(temp2.W(), temp3.W(), MemOperand(temp3));
  __ Orr(temp1.W(), temp3.W(), Operand(temp1.W(), LSL, 20));
  __ Bfi(temp2, temp1, 32, 32);
  __ Fmov(double_temp1, temp2);

  __ Fmul(result, double_temp3, double_temp1);

  __ Bind(&done);
}

#undef __

} }  // namespace v8::internal

#endif  // V8_TARGET_ARCH_ARM64
