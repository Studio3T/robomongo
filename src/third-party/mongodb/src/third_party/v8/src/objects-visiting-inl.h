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

#ifndef V8_OBJECTS_VISITING_INL_H_
#define V8_OBJECTS_VISITING_INL_H_


namespace v8 {
namespace internal {

template<typename StaticVisitor>
void StaticNewSpaceVisitor<StaticVisitor>::Initialize() {
  table_.Register(kVisitShortcutCandidate,
                  &FixedBodyVisitor<StaticVisitor,
                  ConsString::BodyDescriptor,
                  int>::Visit);

  table_.Register(kVisitConsString,
                  &FixedBodyVisitor<StaticVisitor,
                  ConsString::BodyDescriptor,
                  int>::Visit);

  table_.Register(kVisitSlicedString,
                  &FixedBodyVisitor<StaticVisitor,
                  SlicedString::BodyDescriptor,
                  int>::Visit);

  table_.Register(kVisitFixedArray,
                  &FlexibleBodyVisitor<StaticVisitor,
                  FixedArray::BodyDescriptor,
                  int>::Visit);

  table_.Register(kVisitFixedDoubleArray, &VisitFixedDoubleArray);

  table_.Register(kVisitGlobalContext,
                  &FixedBodyVisitor<StaticVisitor,
                  Context::ScavengeBodyDescriptor,
                  int>::Visit);

  table_.Register(kVisitByteArray, &VisitByteArray);

  table_.Register(kVisitSharedFunctionInfo,
                  &FixedBodyVisitor<StaticVisitor,
                  SharedFunctionInfo::BodyDescriptor,
                  int>::Visit);

  table_.Register(kVisitSeqAsciiString, &VisitSeqAsciiString);

  table_.Register(kVisitSeqTwoByteString, &VisitSeqTwoByteString);

  table_.Register(kVisitJSFunction, &VisitJSFunction);

  table_.Register(kVisitFreeSpace, &VisitFreeSpace);

  table_.Register(kVisitJSWeakMap, &JSObjectVisitor::Visit);

  table_.Register(kVisitJSRegExp, &JSObjectVisitor::Visit);

  table_.template RegisterSpecializations<DataObjectVisitor,
                                          kVisitDataObject,
                                          kVisitDataObjectGeneric>();

  table_.template RegisterSpecializations<JSObjectVisitor,
                                          kVisitJSObject,
                                          kVisitJSObjectGeneric>();
  table_.template RegisterSpecializations<StructVisitor,
                                          kVisitStruct,
                                          kVisitStructGeneric>();
}


template<typename StaticVisitor>
void StaticMarkingVisitor<StaticVisitor>::Initialize() {
  table_.Register(kVisitShortcutCandidate,
                  &FixedBodyVisitor<StaticVisitor,
                  ConsString::BodyDescriptor,
                  void>::Visit);

  table_.Register(kVisitConsString,
                  &FixedBodyVisitor<StaticVisitor,
                  ConsString::BodyDescriptor,
                  void>::Visit);

  table_.Register(kVisitSlicedString,
                  &FixedBodyVisitor<StaticVisitor,
                  SlicedString::BodyDescriptor,
                  void>::Visit);

  table_.Register(kVisitFixedArray,
                  &FlexibleBodyVisitor<StaticVisitor,
                  FixedArray::BodyDescriptor,
                  void>::Visit);

  table_.Register(kVisitFixedDoubleArray, &DataObjectVisitor::Visit);

  table_.Register(kVisitGlobalContext, &VisitGlobalContext);

  table_.Register(kVisitByteArray, &DataObjectVisitor::Visit);

  table_.Register(kVisitFreeSpace, &DataObjectVisitor::Visit);

  table_.Register(kVisitSeqAsciiString, &DataObjectVisitor::Visit);

  table_.Register(kVisitSeqTwoByteString, &DataObjectVisitor::Visit);

  table_.Register(kVisitJSWeakMap, &StaticVisitor::VisitJSWeakMap);

  table_.Register(kVisitOddball,
                  &FixedBodyVisitor<StaticVisitor,
                  Oddball::BodyDescriptor,
                  void>::Visit);

  table_.Register(kVisitMap,
                  &FixedBodyVisitor<StaticVisitor,
                  Map::BodyDescriptor,
                  void>::Visit);

  table_.Register(kVisitCode, &StaticVisitor::VisitCode);

  // Registration for kVisitSharedFunctionInfo is done by StaticVisitor.

  // Registration for kVisitJSFunction is done by StaticVisitor.

  // Registration for kVisitJSRegExp is done by StaticVisitor.

  table_.Register(kVisitPropertyCell,
                  &FixedBodyVisitor<StaticVisitor,
                  JSGlobalPropertyCell::BodyDescriptor,
                  void>::Visit);

  table_.template RegisterSpecializations<DataObjectVisitor,
                                          kVisitDataObject,
                                          kVisitDataObjectGeneric>();

  table_.template RegisterSpecializations<JSObjectVisitor,
                                          kVisitJSObject,
                                          kVisitJSObjectGeneric>();

  table_.template RegisterSpecializations<StructObjectVisitor,
                                          kVisitStruct,
                                          kVisitStructGeneric>();
}


template<typename StaticVisitor>
void StaticMarkingVisitor<StaticVisitor>::VisitCodeEntry(
    Heap* heap, Address entry_address) {
  Code* code = Code::cast(Code::GetObjectFromEntryAddress(entry_address));
  heap->mark_compact_collector()->RecordCodeEntrySlot(entry_address, code);
  StaticVisitor::MarkObject(heap, code);
}


template<typename StaticVisitor>
void StaticMarkingVisitor<StaticVisitor>::VisitGlobalPropertyCell(
    Heap* heap, RelocInfo* rinfo) {
  ASSERT(rinfo->rmode() == RelocInfo::GLOBAL_PROPERTY_CELL);
  JSGlobalPropertyCell* cell = rinfo->target_cell();
  StaticVisitor::MarkObject(heap, cell);
}


template<typename StaticVisitor>
void StaticMarkingVisitor<StaticVisitor>::VisitDebugTarget(
    Heap* heap, RelocInfo* rinfo) {
  ASSERT((RelocInfo::IsJSReturn(rinfo->rmode()) &&
          rinfo->IsPatchedReturnSequence()) ||
         (RelocInfo::IsDebugBreakSlot(rinfo->rmode()) &&
          rinfo->IsPatchedDebugBreakSlotSequence()));
  Code* target = Code::GetCodeFromTargetAddress(rinfo->call_address());
  heap->mark_compact_collector()->RecordRelocSlot(rinfo, target);
  StaticVisitor::MarkObject(heap, target);
}


template<typename StaticVisitor>
void StaticMarkingVisitor<StaticVisitor>::VisitGlobalContext(
    Map* map, HeapObject* object) {
  FixedBodyVisitor<StaticVisitor,
                   Context::MarkCompactBodyDescriptor,
                   void>::Visit(map, object);

  MarkCompactCollector* collector = map->GetHeap()->mark_compact_collector();
  for (int idx = Context::FIRST_WEAK_SLOT;
       idx < Context::GLOBAL_CONTEXT_SLOTS;
       ++idx) {
    Object** slot =
        HeapObject::RawField(object, FixedArray::OffsetOfElementAt(idx));
    collector->RecordSlot(slot, slot, *slot);
  }
}


template<typename StaticVisitor>
void StaticMarkingVisitor<StaticVisitor>::VisitJSRegExp(
    Map* map, HeapObject* object) {
  int last_property_offset =
      JSRegExp::kSize + kPointerSize * map->inobject_properties();
  StaticVisitor::VisitPointers(map->GetHeap(),
      HeapObject::RawField(object, JSRegExp::kPropertiesOffset),
      HeapObject::RawField(object, last_property_offset));
}


void Code::CodeIterateBody(ObjectVisitor* v) {
  int mode_mask = RelocInfo::kCodeTargetMask |
                  RelocInfo::ModeMask(RelocInfo::EMBEDDED_OBJECT) |
                  RelocInfo::ModeMask(RelocInfo::GLOBAL_PROPERTY_CELL) |
                  RelocInfo::ModeMask(RelocInfo::EXTERNAL_REFERENCE) |
                  RelocInfo::ModeMask(RelocInfo::JS_RETURN) |
                  RelocInfo::ModeMask(RelocInfo::DEBUG_BREAK_SLOT) |
                  RelocInfo::ModeMask(RelocInfo::RUNTIME_ENTRY);

  // There are two places where we iterate code bodies: here and the
  // templated CodeIterateBody (below).  They should be kept in sync.
  IteratePointer(v, kRelocationInfoOffset);
  IteratePointer(v, kHandlerTableOffset);
  IteratePointer(v, kDeoptimizationDataOffset);
  IteratePointer(v, kTypeFeedbackInfoOffset);

  RelocIterator it(this, mode_mask);
  for (; !it.done(); it.next()) {
    it.rinfo()->Visit(v);
  }
}


template<typename StaticVisitor>
void Code::CodeIterateBody(Heap* heap) {
  int mode_mask = RelocInfo::kCodeTargetMask |
                  RelocInfo::ModeMask(RelocInfo::EMBEDDED_OBJECT) |
                  RelocInfo::ModeMask(RelocInfo::GLOBAL_PROPERTY_CELL) |
                  RelocInfo::ModeMask(RelocInfo::EXTERNAL_REFERENCE) |
                  RelocInfo::ModeMask(RelocInfo::JS_RETURN) |
                  RelocInfo::ModeMask(RelocInfo::DEBUG_BREAK_SLOT) |
                  RelocInfo::ModeMask(RelocInfo::RUNTIME_ENTRY);

  // There are two places where we iterate code bodies: here and the
  // non-templated CodeIterateBody (above).  They should be kept in sync.
  StaticVisitor::VisitPointer(
      heap,
      reinterpret_cast<Object**>(this->address() + kRelocationInfoOffset));
  StaticVisitor::VisitPointer(
      heap,
      reinterpret_cast<Object**>(this->address() + kHandlerTableOffset));
  StaticVisitor::VisitPointer(
      heap,
      reinterpret_cast<Object**>(this->address() + kDeoptimizationDataOffset));
  StaticVisitor::VisitPointer(
      heap,
      reinterpret_cast<Object**>(this->address() + kTypeFeedbackInfoOffset));

  RelocIterator it(this, mode_mask);
  for (; !it.done(); it.next()) {
    it.rinfo()->template Visit<StaticVisitor>(heap);
  }
}


} }  // namespace v8::internal

#endif  // V8_OBJECTS_VISITING_INL_H_
