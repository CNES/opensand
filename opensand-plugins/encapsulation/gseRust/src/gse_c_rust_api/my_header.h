#include <cstdarg>
#include <cstdint>
#include <cstdlib>
#include <ostream>
#include <new>

enum class CHeaderExtensionType {
  NoDataExtension,
  Data2B,
  Data4B,
  Data6B,
  Data8B,
};

enum class RustDecapStatusType {
  DecapCompletedPkt,
  DecapFragmentedPkt,
  DecapPadding,
  DecapErrorSizeBuffer,
  DecapErrorTotalLength,
  DecapErrorGseLength,
  DecapErrorSizePduBuffer,
  DecapErrorMemoryStorageOverflow,
  DecapErrorMemoryStorageUnderflow,
  DecapErrorMemoryUndefinedId,
  DecapErrorMemoryBufferTooSmall,
  DecapErrorMemoryMemoryCorrupted,
  DecapErrorCRC,
  DecapErrorProtocolType,
  DecapErrorInvalidLabel,
  DecapErrorNoLabelSaved,
  ErrorLabelBroadcastSaved,
  ErrorLabelReUseSaved,
};

enum class RustEncapStatusType {
  EncapCompletedPkt,
  EncapFragmentedPkt,
  EncapErrorPduLength,
  EncapErrorSizeBuffer,
  EncapErrorSizePduBuffer,
  EncapErrorProtocolType,
  EncapErrorInvalidLabel,
};

enum class RustExtractLabelorFragIdType {
  ResLbl,
  ResFragId,
  ErrorLabelReUse,
  ErrorSizeBuffer,
  ErrorHeaderRead,
  ErrorLabelBroadcast,
};

enum class RustGetExtensionsHeaderType {
  NoExtensionsHeader,
  ExtensionsHeader,
  ErrNoextension,
  ErrReadingPacket,
  ErrBufferTooSmall,
  ErrNotGSEPacket,
};

enum class RustLabelType {
  SixBytes,
  ThreeBytes,
  Broadcast,
  ReUse,
};

using OpaquePtrEncap = Encapsulator<DefaultCrc>;

struct RustContextFrag {
  uint8_t frag_id;
  uint32_t crc;
  uint16_t len_pdu_frag;
};

struct EncapStatusFrag {
  uint16_t len_pkt;
  RustContextFrag context;
};

union RustEncapStatusValue {
  uint16_t completed_pkt;
  EncapStatusFrag fragmented_pkt;
  uint8_t other;
};

struct RustEncapStatus {
  RustEncapStatusType status;
  RustEncapStatusValue value;
};

struct RustSlice {
  size_t size;
  const uint8_t *bytes;
};

struct RustMutSlice {
  size_t size;
  uint8_t *bytes;
};

struct RustLabel {
  RustLabelType label_type;
  uint8_t bytes[6];
};

struct RustEncapMetadata {
  uint16_t protocol_type;
  RustLabel label;
};

struct CHeaderExtension {
  uint16_t id;
  uint8_t data[8];
  CHeaderExtensionType size;
};

struct CHeaderExtensionSlice {
  size_t size;
  const CHeaderExtension *bytes;
};

struct RustDecapContext {
  RustLabel label;
  uint16_t protocol_type;
  uint8_t frag_id;
  uint16_t total_len;
  uint16_t pdu_len;
  bool from_label_reuse;
};

struct RustMemoryContext {
  RustDecapContext context;
  RustMutSlice pdu;
};

struct c_memory {
  RustMemoryContext *frags;
  RustMutSlice *storage;
  size_t max_frag_id;
  size_t max_pdu_size;
};

using OpaquePtrDecap = Decapsulator<c_memory, DefaultCrc>;

struct RustDecapMetadata {
  uint16_t protocol_type;
  RustLabel label;
  size_t pdu_len;
};

struct RustDecapStatusCompleted {
  RustMutSlice pdu;
  RustDecapMetadata metadata;
};

union RustDecapStatusValue {
  RustDecapStatusCompleted completed_pkt;
  uint8_t other;
};

struct RustDecapStatus {
  size_t len_pkt;
  RustDecapStatusType status;
  RustDecapStatusValue value;
};

using FragId = uint8_t;

union RustExtractLabelorFragIdValue {
  FragId fragid;
  RustLabel label;
  uint8_t other;
};

struct RustExtractLabelorFragIdStatus {
  RustExtractLabelorFragIdType status;
  RustExtractLabelorFragIdValue value;
};

struct RustGetExtensionsHeader {
  RustGetExtensionsHeaderType status;
  CHeaderExtensionSlice value;
};

extern "C" {

/// # Safety
/// ## Safety Guarantees
/// the encapsulator should be free using delete_encapsulator
OpaquePtrEncap *create_encapsulator();

/// # Safety
///
/// This function takes a raw pointer to an `OpaquePtrEncap` and deallocates its memory.
///
/// ## Safety Guarantees
/// 1. **Valid Pointer:** The caller must ensure that `ptr` is a valid, non-null pointer
///    to a previously allocated `OpaquePtrEncap`.
/// 2. **Single Ownership:** The caller must ensure that the pointer passed to this function
///    is not used elsewhere after this function is called. This function assumes
///    ownership and will deallocate the memory, making the pointer invalid.
/// 3. **Proper Allocation:** The pointer must have been allocated using Rust's `create_encapsulator`.
///
/// Violating any of these guarantees can result in undefined behavior.
///
void delete_encapsulator(OpaquePtrEncap *ptr);

void enable_labelReUse(OpaquePtrEncap *ptr, bool enable);

RustEncapStatus rust_encap_frag(RustSlice c_pdu,
                                RustContextFrag c_context,
                                RustMutSlice buffer,
                                OpaquePtrEncap *ptr);

RustEncapStatus rust_encap(RustSlice c_pdu,
                           uint8_t frag_id,
                           RustEncapMetadata c_metadata,
                           RustMutSlice c_buffer,
                           OpaquePtrEncap *ptr);

RustEncapStatus rust_encap_ext(RustSlice c_pdu,
                               uint8_t frag_id,
                               RustEncapMetadata c_metadata,
                               RustMutSlice c_buffer,
                               OpaquePtrEncap *ptr,
                               CHeaderExtensionSlice ext);

/// # Safety
///
/// This function takes a raw pointer to an `OpaquePtrDecap` and deallocates its memory.
///
/// ## Safety Guarantees
/// 1. **Valid Pointer:** The caller must ensure that `ptr` is a valid, non-null pointer
///    to a previously allocated `OpaquePtrDecap`.
/// 2. **Single Ownership:** The caller must ensure that the pointer passed to this function
///    is not used elsewhere after this function is called. This function assumes
///    ownership and will deallocate the memory, making the pointer invalid.
/// 3. **Proper Allocation:** The pointer must have been allocated using Rust's `create_deencapsulator`.
///
/// Violating any of these guarantees can result in undefined behavior.
void delete_deencapsulator(OpaquePtrDecap *ptr);

/// # Safety
/// ## Safety Guarantees
/// the deencapsulator should be free using delete_deencapsulator
OpaquePtrDecap *create_deencapsulator(c_memory decap_buffer);

RustDecapStatus rust_decap(RustSlice c_buffer, OpaquePtrDecap *ptr);

RustExtractLabelorFragIdStatus rust_getFragIdOrLbl(RustSlice c_buffer, OpaquePtrDecap *ptr);

RustGetExtensionsHeader rust_getExtensionHeaders(RustSlice c_buffer, OpaquePtrDecap *ptr);

} // extern "C"
