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

#ifndef V8_SERIALIZE_H_
#define V8_SERIALIZE_H_

#include "hashmap.h"

namespace v8 {
namespace internal {

// A TypeCode is used to distinguish different kinds of external reference.
// It is a single bit to make testing for types easy.
enum TypeCode {
  UNCLASSIFIED,        // One-of-a-kind references.
  BUILTIN,
  RUNTIME_FUNCTION,
  IC_UTILITY,
  DEBUG_ADDRESS,
  STATS_COUNTER,
  TOP_ADDRESS,
  C_BUILTIN,
  EXTENSION,
  ACCESSOR,
  RUNTIME_ENTRY,
  STUB_CACHE_TABLE
};

const int kTypeCodeCount = STUB_CACHE_TABLE + 1;
const int kFirstTypeCode = UNCLASSIFIED;

const int kReferenceIdBits = 16;
const int kReferenceIdMask = (1 << kReferenceIdBits) - 1;
const int kReferenceTypeShift = kReferenceIdBits;
const int kDebugRegisterBits = 4;
const int kDebugIdShift = kDebugRegisterBits;


// ExternalReferenceTable is a helper class that defines the relationship
// between external references and their encodings. It is used to build
// hashmaps in ExternalReferenceEncoder and ExternalReferenceDecoder.
class ExternalReferenceTable {
 public:
  static ExternalReferenceTable* instance(Isolate* isolate);

  ~ExternalReferenceTable() { }

  int size() const { return refs_.length(); }

  Address address(int i) { return refs_[i].address; }

  uint32_t code(int i) { return refs_[i].code; }

  const char* name(int i) { return refs_[i].name; }

  int max_id(int code) { return max_id_[code]; }

 private:
  explicit ExternalReferenceTable(Isolate* isolate) : refs_(64) {
      PopulateTable(isolate);
  }

  struct ExternalReferenceEntry {
    Address address;
    uint32_t code;
    const char* name;
  };

  void PopulateTable(Isolate* isolate);

  // For a few types of references, we can get their address from their id.
  void AddFromId(TypeCode type,
                 uint16_t id,
                 const char* name,
                 Isolate* isolate);

  // For other types of references, the caller will figure out the address.
  void Add(Address address, TypeCode type, uint16_t id, const char* name);

  List<ExternalReferenceEntry> refs_;
  int max_id_[kTypeCodeCount];
};


class ExternalReferenceEncoder {
 public:
  ExternalReferenceEncoder();

  uint32_t Encode(Address key) const;

  const char* NameOfAddress(Address key) const;

 private:
  HashMap encodings_;
  static uint32_t Hash(Address key) {
    return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(key) >> 2);
  }

  int IndexOf(Address key) const;

  static bool Match(void* key1, void* key2) { return key1 == key2; }

  void Put(Address key, int index);

  Isolate* isolate_;
};


class ExternalReferenceDecoder {
 public:
  ExternalReferenceDecoder();
  ~ExternalReferenceDecoder();

  Address Decode(uint32_t key) const {
    if (key == 0) return NULL;
    return *Lookup(key);
  }

 private:
  Address** encodings_;

  Address* Lookup(uint32_t key) const {
    int type = key >> kReferenceTypeShift;
    ASSERT(kFirstTypeCode <= type && type < kTypeCodeCount);
    int id = key & kReferenceIdMask;
    return &encodings_[type][id];
  }

  void Put(uint32_t key, Address value) {
    *Lookup(key) = value;
  }

  Isolate* isolate_;
};


class SnapshotByteSource {
 public:
  SnapshotByteSource(const byte* array, int length)
    : data_(array), length_(length), position_(0) { }

  bool HasMore() { return position_ < length_; }

  int Get() {
    ASSERT(position_ < length_);
    return data_[position_++];
  }

  inline void CopyRaw(byte* to, int number_of_bytes);

  inline int GetInt();

  bool AtEOF() {
    return position_ == length_;
  }

  int position() { return position_; }

 private:
  const byte* data_;
  int length_;
  int position_;
};


#define COMMON_RAW_LENGTHS(f)        \
  f(1, 1)  \
  f(2, 2)  \
  f(3, 3)  \
  f(4, 4)  \
  f(5, 5)  \
  f(6, 6)  \
  f(7, 7)  \
  f(8, 8)  \
  f(9, 12)  \
  f(10, 16) \
  f(11, 20) \
  f(12, 24) \
  f(13, 28) \
  f(14, 32) \
  f(15, 36)

// The Serializer/Deserializer class is a common superclass for Serializer and
// Deserializer which is used to store common constants and methods used by
// both.
class SerializerDeserializer: public ObjectVisitor {
 public:
  static void Iterate(ObjectVisitor* visitor);

 protected:
  // Where the pointed-to object can be found:
  enum Where {
    kNewObject = 0,                 // Object is next in snapshot.
    // 1-8                             One per space.
    kRootArray = 0x9,               // Object is found in root array.
    kPartialSnapshotCache = 0xa,    // Object is in the cache.
    kExternalReference = 0xb,       // Pointer to an external reference.
    kSkip = 0xc,                    // Skip a pointer sized cell.
    // 0xd-0xf                         Free.
    kBackref = 0x10,                 // Object is described relative to end.
    // 0x11-0x18                       One per space.
    // 0x19-0x1f                       Free.
    kFromStart = 0x20,              // Object is described relative to start.
    // 0x21-0x28                       One per space.
    // 0x29-0x2f                       Free.
    // 0x30-0x3f                       Used by misc. tags below.
    kPointedToMask = 0x3f
  };

  // How to code the pointer to the object.
  enum HowToCode {
    kPlain = 0,                          // Straight pointer.
    // What this means depends on the architecture:
    kFromCode = 0x40,                    // A pointer inlined in code.
    kHowToCodeMask = 0x40
  };

  // Where to point within the object.
  enum WhereToPoint {
    kStartOfObject = 0,
    kInnerPointer = 0x80,  // First insn in code object or payload of cell.
    kWhereToPointMask = 0x80
  };

  // Misc.
  // Raw data to be copied from the snapshot.
  static const int kRawData = 0x30;
  // Some common raw lengths: 0x31-0x3f
  // A tag emitted at strategic points in the snapshot to delineate sections.
  // If the deserializer does not find these at the expected moments then it
  // is an indication that the snapshot and the VM do not fit together.
  // Examine the build process for architecture, version or configuration
  // mismatches.
  static const int kSynchronize = 0x70;
  // Used for the source code of the natives, which is in the executable, but
  // is referred to from external strings in the snapshot.
  static const int kNativesStringResource = 0x71;
  static const int kNewPage = 0x72;
  static const int kRepeat = 0x73;
  static const int kConstantRepeat = 0x74;
  // 0x74-0x7f            Repeat last word (subtract 0x73 to get the count).
  static const int kMaxRepeats = 0x7f - 0x73;
  static int CodeForRepeats(int repeats) {
    ASSERT(repeats >= 1 && repeats <= kMaxRepeats);
    return 0x73 + repeats;
  }
  static int RepeatsForCode(int byte_code) {
    ASSERT(byte_code >= kConstantRepeat && byte_code <= 0x7f);
    return byte_code - 0x73;
  }
  static const int kRootArrayLowConstants = 0xb0;
  // 0xb0-0xbf            Things from the first 16 elements of the root array.
  static const int kRootArrayHighConstants = 0xf0;
  // 0xf0-0xff            Things from the next 16 elements of the root array.
  static const int kRootArrayNumberOfConstantEncodings = 0x20;
  static const int kRootArrayNumberOfLowConstantEncodings = 0x10;
  static int RootArrayConstantFromByteCode(int byte_code) {
    int constant = (byte_code & 0xf) | ((byte_code & 0x40) >> 2);
    ASSERT(constant >= 0 && constant < kRootArrayNumberOfConstantEncodings);
    return constant;
  }


  static const int kLargeData = LAST_SPACE;
  static const int kLargeCode = kLargeData + 1;
  static const int kLargeFixedArray = kLargeCode + 1;
  static const int kNumberOfSpaces = kLargeFixedArray + 1;
  static const int kAnyOldSpace = -1;

  // A bitmask for getting the space out of an instruction.
  static const int kSpaceMask = 15;

  static inline bool SpaceIsLarge(int space) { return space >= kLargeData; }
  static inline bool SpaceIsPaged(int space) {
    return space >= FIRST_PAGED_SPACE && space <= LAST_PAGED_SPACE;
  }
};


int SnapshotByteSource::GetInt() {
  // A little unwind to catch the really small ints.
  int snapshot_byte = Get();
  if ((snapshot_byte & 0x80) == 0) {
    return snapshot_byte;
  }
  int accumulator = (snapshot_byte & 0x7f) << 7;
  while (true) {
    snapshot_byte = Get();
    if ((snapshot_byte & 0x80) == 0) {
      return accumulator | snapshot_byte;
    }
    accumulator = (accumulator | (snapshot_byte & 0x7f)) << 7;
  }
  UNREACHABLE();
  return accumulator;
}


void SnapshotByteSource::CopyRaw(byte* to, int number_of_bytes) {
  memcpy(to, data_ + position_, number_of_bytes);
  position_ += number_of_bytes;
}


// A Deserializer reads a snapshot and reconstructs the Object graph it defines.
class Deserializer: public SerializerDeserializer {
 public:
  // Create a deserializer from a snapshot byte source.
  explicit Deserializer(SnapshotByteSource* source);

  virtual ~Deserializer();

  // Deserialize the snapshot into an empty heap.
  void Deserialize();

  // Deserialize a single object and the objects reachable from it.
  void DeserializePartial(Object** root);

 private:
  virtual void VisitPointers(Object** start, Object** end);

  virtual void VisitExternalReferences(Address* start, Address* end) {
    UNREACHABLE();
  }

  virtual void VisitRuntimeEntry(RelocInfo* rinfo) {
    UNREACHABLE();
  }

  // Fills in some heap data in an area from start to end (non-inclusive).  The
  // space id is used for the write barrier.  The object_address is the address
  // of the object we are writing into, or NULL if we are not writing into an
  // object, i.e. if we are writing a series of tagged values that are not on
  // the heap.
  void ReadChunk(
      Object** start, Object** end, int space, Address object_address);
  HeapObject* GetAddressFromStart(int space);
  inline HeapObject* GetAddressFromEnd(int space);
  Address Allocate(int space_number, Space* space, int size);
  void ReadObject(int space_number, Space* space, Object** write_back);

  // Cached current isolate.
  Isolate* isolate_;

  // Keep track of the pages in the paged spaces.
  // (In large object space we are keeping track of individual objects
  // rather than pages.)  In new space we just need the address of the
  // first object and the others will flow from that.
  List<Address> pages_[SerializerDeserializer::kNumberOfSpaces];

  SnapshotByteSource* source_;
  // This is the address of the next object that will be allocated in each
  // space.  It is used to calculate the addresses of back-references.
  Address high_water_[LAST_SPACE + 1];
  // This is the address of the most recent object that was allocated.  It
  // is used to set the location of the new page when we encounter a
  // START_NEW_PAGE_SERIALIZATION tag.
  Address last_object_address_;

  ExternalReferenceDecoder* external_reference_decoder_;

  DISALLOW_COPY_AND_ASSIGN(Deserializer);
};


class SnapshotByteSink {
 public:
  virtual ~SnapshotByteSink() { }
  virtual void Put(int byte, const char* description) = 0;
  virtual void PutSection(int byte, const char* description) {
    Put(byte, description);
  }
  void PutInt(uintptr_t integer, const char* description);
  virtual int Position() = 0;
};


// Mapping objects to their location after deserialization.
// This is used during building, but not at runtime by V8.
class SerializationAddressMapper {
 public:
  SerializationAddressMapper()
      : serialization_map_(new HashMap(&SerializationMatchFun)),
        no_allocation_(new AssertNoAllocation()) { }

  ~SerializationAddressMapper() {
    delete serialization_map_;
    delete no_allocation_;
  }

  bool IsMapped(HeapObject* obj) {
    return serialization_map_->Lookup(Key(obj), Hash(obj), false) != NULL;
  }

  int MappedTo(HeapObject* obj) {
    ASSERT(IsMapped(obj));
    return static_cast<int>(reinterpret_cast<intptr_t>(
        serialization_map_->Lookup(Key(obj), Hash(obj), false)->value));
  }

  void AddMapping(HeapObject* obj, int to) {
    ASSERT(!IsMapped(obj));
    HashMap::Entry* entry =
        serialization_map_->Lookup(Key(obj), Hash(obj), true);
    entry->value = Value(to);
  }

 private:
  static bool SerializationMatchFun(void* key1, void* key2) {
    return key1 == key2;
  }

  static uint32_t Hash(HeapObject* obj) {
    return static_cast<int32_t>(reinterpret_cast<intptr_t>(obj->address()));
  }

  static void* Key(HeapObject* obj) {
    return reinterpret_cast<void*>(obj->address());
  }

  static void* Value(int v) {
    return reinterpret_cast<void*>(v);
  }

  HashMap* serialization_map_;
  AssertNoAllocation* no_allocation_;
  DISALLOW_COPY_AND_ASSIGN(SerializationAddressMapper);
};


// There can be only one serializer per V8 process.
class Serializer : public SerializerDeserializer {
 public:
  explicit Serializer(SnapshotByteSink* sink);
  ~Serializer();
  void VisitPointers(Object** start, Object** end);
  // You can call this after serialization to find out how much space was used
  // in each space.
  int CurrentAllocationAddress(int space) {
    if (SpaceIsLarge(space)) return large_object_total_;
    return fullness_[space];
  }

  static void Enable() {
    if (!serialization_enabled_) {
      ASSERT(!too_late_to_enable_now_);
    }
    serialization_enabled_ = true;
  }

  static void Disable() { serialization_enabled_ = false; }
  // Call this when you have made use of the fact that there is no serialization
  // going on.
  static void TooLateToEnableNow() { too_late_to_enable_now_ = true; }
  static bool enabled() { return serialization_enabled_; }
  SerializationAddressMapper* address_mapper() { return &address_mapper_; }
  void PutRoot(
      int index, HeapObject* object, HowToCode how, WhereToPoint where);

 protected:
  static const int kInvalidRootIndex = -1;

  int RootIndex(HeapObject* heap_object, HowToCode from);
  virtual bool ShouldBeInThePartialSnapshotCache(HeapObject* o) = 0;
  intptr_t root_index_wave_front() { return root_index_wave_front_; }
  void set_root_index_wave_front(intptr_t value) {
    ASSERT(value >= root_index_wave_front_);
    root_index_wave_front_ = value;
  }

  class ObjectSerializer : public ObjectVisitor {
   public:
    ObjectSerializer(Serializer* serializer,
                     Object* o,
                     SnapshotByteSink* sink,
                     HowToCode how_to_code,
                     WhereToPoint where_to_point)
      : serializer_(serializer),
        object_(HeapObject::cast(o)),
        sink_(sink),
        reference_representation_(how_to_code + where_to_point),
        bytes_processed_so_far_(0) { }
    void Serialize();
    void VisitPointers(Object** start, Object** end);
    void VisitEmbeddedPointer(RelocInfo* target);
    void VisitExternalReferences(Address* start, Address* end);
    void VisitExternalReference(RelocInfo* rinfo);
    void VisitCodeTarget(RelocInfo* target);
    void VisitCodeEntry(Address entry_address);
    void VisitGlobalPropertyCell(RelocInfo* rinfo);
    void VisitRuntimeEntry(RelocInfo* reloc);
    // Used for seralizing the external strings that hold the natives source.
    void VisitExternalAsciiString(
        v8::String::ExternalAsciiStringResource** resource);
    // We can't serialize a heap with external two byte strings.
    void VisitExternalTwoByteString(
        v8::String::ExternalStringResource** resource) {
      UNREACHABLE();
    }

   private:
    void OutputRawData(Address up_to);

    Serializer* serializer_;
    HeapObject* object_;
    SnapshotByteSink* sink_;
    int reference_representation_;
    int bytes_processed_so_far_;
  };

  virtual void SerializeObject(Object* o,
                               HowToCode how_to_code,
                               WhereToPoint where_to_point) = 0;
  void SerializeReferenceToPreviousObject(
      int space,
      int address,
      HowToCode how_to_code,
      WhereToPoint where_to_point);
  void InitializeAllocators();
  // This will return the space for an object.  If the object is in large
  // object space it may return kLargeCode or kLargeFixedArray in order
  // to indicate to the deserializer what kind of large object allocation
  // to make.
  static int SpaceOfObject(HeapObject* object);
  // This just returns the space of the object.  It will return LO_SPACE
  // for all large objects since you can't check the type of the object
  // once the map has been used for the serialization address.
  static int SpaceOfAlreadySerializedObject(HeapObject* object);
  int Allocate(int space, int size, bool* new_page_started);
  int EncodeExternalReference(Address addr) {
    return external_reference_encoder_->Encode(addr);
  }

  int SpaceAreaSize(int space);

  Isolate* isolate_;
  // Keep track of the fullness of each space in order to generate
  // relative addresses for back references.  Large objects are
  // just numbered sequentially since relative addresses make no
  // sense in large object space.
  int fullness_[LAST_SPACE + 1];
  SnapshotByteSink* sink_;
  int current_root_index_;
  ExternalReferenceEncoder* external_reference_encoder_;
  static bool serialization_enabled_;
  // Did we already make use of the fact that serialization was not enabled?
  static bool too_late_to_enable_now_;
  int large_object_total_;
  SerializationAddressMapper address_mapper_;
  intptr_t root_index_wave_front_;

  friend class ObjectSerializer;
  friend class Deserializer;

 private:
  DISALLOW_COPY_AND_ASSIGN(Serializer);
};


class PartialSerializer : public Serializer {
 public:
  PartialSerializer(Serializer* startup_snapshot_serializer,
                    SnapshotByteSink* sink)
    : Serializer(sink),
      startup_serializer_(startup_snapshot_serializer) {
    set_root_index_wave_front(Heap::kStrongRootListLength);
  }

  // Serialize the objects reachable from a single object pointer.
  virtual void Serialize(Object** o);
  virtual void SerializeObject(Object* o,
                               HowToCode how_to_code,
                               WhereToPoint where_to_point);

 protected:
  virtual int PartialSnapshotCacheIndex(HeapObject* o);
  virtual bool ShouldBeInThePartialSnapshotCache(HeapObject* o) {
    // Scripts should be referred only through shared function infos.  We can't
    // allow them to be part of the partial snapshot because they contain a
    // unique ID, and deserializing several partial snapshots containing script
    // would cause dupes.
    ASSERT(!o->IsScript());
    return o->IsString() || o->IsSharedFunctionInfo() ||
           o->IsHeapNumber() || o->IsCode() ||
           o->IsScopeInfo() ||
           o->map() == HEAP->fixed_cow_array_map();
  }

 private:
  Serializer* startup_serializer_;
  DISALLOW_COPY_AND_ASSIGN(PartialSerializer);
};


class StartupSerializer : public Serializer {
 public:
  explicit StartupSerializer(SnapshotByteSink* sink) : Serializer(sink) {
    // Clear the cache of objects used by the partial snapshot.  After the
    // strong roots have been serialized we can create a partial snapshot
    // which will repopulate the cache with objects needed by that partial
    // snapshot.
    Isolate::Current()->set_serialize_partial_snapshot_cache_length(0);
  }
  // Serialize the current state of the heap.  The order is:
  // 1) Strong references.
  // 2) Partial snapshot cache.
  // 3) Weak references (e.g. the symbol table).
  virtual void SerializeStrongReferences();
  virtual void SerializeObject(Object* o,
                               HowToCode how_to_code,
                               WhereToPoint where_to_point);
  void SerializeWeakReferences();
  void Serialize() {
    SerializeStrongReferences();
    SerializeWeakReferences();
  }

 private:
  virtual bool ShouldBeInThePartialSnapshotCache(HeapObject* o) {
    return false;
  }
};


} }  // namespace v8::internal

#endif  // V8_SERIALIZE_H_
