use crate::common::{cslice_from_rsslice, new_label_fromrs, RustLabel, RustMutSlice, RustSlice};
use crate::gse_decap_memory::c_memory;
use crate::header_ext::{from_c_vec_to_rust_vec, from_rust_vec_to_c_vec, CHeaderExtensionSlice};
use dvb_gse_rust::crc::DefaultCrc;
use dvb_gse_rust::gse_decap::{gse_decap_memory::MemoryContext, DecapContext};
use dvb_gse_rust::gse_decap::{
    DecapError, DecapMetadata, DecapStatus, Decapsulator, GetLabelorFragIdError, LabelorFragId,
};
use libc::size_t;
use std::mem::ManuallyDrop;

#[repr(C)]
#[derive(Clone)]
pub struct RustDecapContext {
    label: RustLabel,
    protocol_type: u16,
    frag_id: u8,
    total_len: u16,
    pdu_len: u16,
    from_label_reuse: bool,
    header_extensions: CHeaderExtensionSlice,
}

impl RustDecapContext {
    pub fn to_rust_decap_context(&self) -> DecapContext {
        DecapContext {
            label: self.label.to_rust_label(),
            protocol_type: self.protocol_type,
            frag_id: self.frag_id,
            total_len: self.total_len,
            pdu_len: self.pdu_len,
            from_label_reuse: self.from_label_reuse,
            extensions_header: from_c_vec_to_rust_vec(self.header_extensions.to_slice()),
        }
    }
}

pub fn new_decap_context_fromrs(rs_context: DecapContext) -> RustDecapContext {
    RustDecapContext {
        label: new_label_fromrs(rs_context.label),
        protocol_type: rs_context.protocol_type,
        frag_id: rs_context.frag_id,
        total_len: rs_context.total_len,
        pdu_len: rs_context.pdu_len,
        from_label_reuse: rs_context.from_label_reuse,
        header_extensions: CHeaderExtensionSlice::from(from_rust_vec_to_c_vec(
            &rs_context.extensions_header,
        )),
    }
}

#[repr(C)]
pub struct RustMemoryContext {
    context: RustDecapContext,
    pub pdu: RustMutSlice,
}

impl RustMemoryContext {
    pub fn to_rust_memory_context(&self) -> MemoryContext {
        (self.context.to_rust_decap_context(), self.pdu.to_box())
    }
}

pub fn new_memory_context_fromrs(rs_context: MemoryContext) -> RustMemoryContext {
    let mut pdu_box = ManuallyDrop::new(rs_context.1);
    RustMemoryContext {
        context: new_decap_context_fromrs(rs_context.0),
        pdu: cslice_from_rsslice(&mut pdu_box),
    }
}

pub type OpaquePtrDecap = Decapsulator<c_memory, DefaultCrc, SimpleMandatoryExtensionHeaderManager>;

#[derive(Copy, Clone)]
#[repr(C)]
pub struct RustDecapMetadata {
    protocol_type: u16,
    label: RustLabel,
    pdu_len: size_t,
    extensions: CHeaderExtensionSlice,
}

#[repr(C)]
#[derive(Copy, Clone)]
pub struct RustDecapStatus {
    len_pkt: size_t,
    status: RustDecapStatusType,
    value: RustDecapStatusValue,
}

#[repr(C)]
#[derive(Copy, Clone)]
pub union RustDecapStatusValue {
    completed_pkt: RustDecapStatusCompleted,
    other: u8,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct RustDecapStatusCompleted {
    pdu: RustMutSlice,
    metadata: RustDecapMetadata,
}

#[repr(C)]
#[derive(Copy, Clone)]
pub enum RustDecapStatusType {
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
    DecapErrorUnkownMandatoryHeader,
}

fn new_decap_metadata_fromrs(rs_metadata: DecapMetadata) -> RustDecapMetadata {
    RustDecapMetadata {
        protocol_type: rs_metadata.protocol_type(),
        label: new_label_fromrs(rs_metadata.label()),
        pdu_len: rs_metadata.pdu_len(),
        extensions: CHeaderExtensionSlice::from(from_rust_vec_to_c_vec(rs_metadata.extensions())),
    }
}

fn new_decap_status_from_rust_status(rs_status: DecapStatus, len_pkt: usize) -> RustDecapStatus {
    let c_status: RustDecapStatus = match rs_status {
        DecapStatus::CompletedPkt(rs_pdu, rs_metadata) => {
            let mut rs_pdu = ManuallyDrop::new(rs_pdu);
            return RustDecapStatus {
                len_pkt,
                status: RustDecapStatusType::DecapCompletedPkt,
                value: RustDecapStatusValue {
                    completed_pkt: RustDecapStatusCompleted {
                        pdu: RustMutSlice {
                            size: rs_pdu.len(),
                            bytes: rs_pdu.as_mut_ptr(),
                        },
                        metadata: new_decap_metadata_fromrs(rs_metadata),
                    },
                },
            };
        }
        DecapStatus::FragmentedPkt(_) => RustDecapStatus { // TODO return MetaData
            len_pkt,
            status: RustDecapStatusType::DecapFragmentedPkt,
            value: RustDecapStatusValue { other: 0 },
        },
        DecapStatus::Padding => RustDecapStatus {
            len_pkt,
            status: RustDecapStatusType::DecapPadding,
            value: RustDecapStatusValue { other: 0 },
        },
    };
    c_status
}

fn new_decap_status_from_rust_error(rs_status: DecapError, len_pkt: usize) -> RustDecapStatus {
    let c_status: RustDecapStatus = match rs_status {
        DecapError::ErrorCrc => RustDecapStatus {
            len_pkt,
            status: RustDecapStatusType::DecapErrorCRC,
            value: RustDecapStatusValue { other: 0 },
        },
        DecapError::ErrorSizeBuffer => RustDecapStatus {
            len_pkt,
            status: RustDecapStatusType::DecapErrorSizeBuffer,
            value: RustDecapStatusValue { other: 0 },
        },
        DecapError::ErrorTotalLength => RustDecapStatus {
            len_pkt,
            status: RustDecapStatusType::DecapErrorTotalLength,
            value: RustDecapStatusValue { other: 0 },
        },
        DecapError::ErrorGseLength => RustDecapStatus {
            len_pkt,
            status: RustDecapStatusType::DecapErrorGseLength,
            value: RustDecapStatusValue { other: 0 },
        },
        DecapError::ErrorSizePduBuffer => RustDecapStatus {
            len_pkt,
            status: RustDecapStatusType::DecapErrorSizePduBuffer,
            value: RustDecapStatusValue { other: 0 },
        },
        DecapError::ErrorProtocolType => RustDecapStatus {
            len_pkt,
            status: RustDecapStatusType::DecapErrorProtocolType,
            value: RustDecapStatusValue { other: 0 },
        },
        DecapError::ErrorUnkownMandatoryHeader => RustDecapStatus {
            len_pkt,
            status: RustDecapStatusType::DecapErrorUnkownMandatoryHeader,
            value: RustDecapStatusValue { other: 0 },
        },

        //TODO maybe display more info about the DecapMemoryError
        DecapError::ErrorMemory(e) => {
            let st: RustDecapStatusType = match e {
                dvb_gse_rust::gse_decap::DecapMemoryError::StorageOverflow(_) => {
                    RustDecapStatusType::DecapErrorMemoryStorageOverflow
                }
                dvb_gse_rust::gse_decap::DecapMemoryError::StorageUnderflow => {
                    RustDecapStatusType::DecapErrorMemoryStorageUnderflow
                }
                dvb_gse_rust::gse_decap::DecapMemoryError::UndefinedId => {
                    RustDecapStatusType::DecapErrorMemoryUndefinedId
                }
                dvb_gse_rust::gse_decap::DecapMemoryError::MemoryCorrupted => {
                    RustDecapStatusType::DecapErrorMemoryMemoryCorrupted
                }
                dvb_gse_rust::gse_decap::DecapMemoryError::BufferTooSmall(_) => {
                    RustDecapStatusType::DecapErrorMemoryBufferTooSmall
                }
            };
            RustDecapStatus {
                len_pkt,
                status: st,
                value: RustDecapStatusValue { other: 0 },
            }
        }
        DecapError::ErrorInvalidLabel => RustDecapStatus {
            len_pkt,
            status: RustDecapStatusType::DecapErrorInvalidLabel,
            value: RustDecapStatusValue { other: 0 },
        },
        DecapError::ErrorNoLabelSaved => RustDecapStatus {
            len_pkt,
            status: RustDecapStatusType::DecapErrorInvalidLabel,
            value: RustDecapStatusValue { other: 0 },
        },
        DecapError::ErrorLabelBroadcastSaved => RustDecapStatus {
            len_pkt,
            status: RustDecapStatusType::DecapErrorSizePduBuffer,
            value: RustDecapStatusValue { other: 0 },
        },
        DecapError::ErrorLabelReUseSaved => RustDecapStatus {
            len_pkt,
            status: RustDecapStatusType::ErrorLabelReUseSaved,
            value: RustDecapStatusValue { other: 0 },
        },
    };
    c_status
}

pub fn get_deencapsulator_from_ptr(
    ptr: &mut OpaquePtrDecap,
) -> Option<&Decapsulator<c_memory, DefaultCrc, SimpleMandatoryExtensionHeaderManager>> {
    // Convertir le pointeur en objet Rust
    let deencap = &mut *ptr;

    Some(deencap)
}

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

#[no_mangle]
pub unsafe extern "C" fn delete_deencapsulator(ptr: *mut OpaquePtrDecap) {
    unsafe { drop(Box::from_raw(ptr)) }
}

/// # Safety
/// ## Safety Guarantees
/// the deencapsulator should be free using delete_deencapsulator
#[no_mangle]
pub extern "C" fn create_deencapsulator(decap_buffer: c_memory) -> *mut OpaquePtrDecap {
    // Convertir le pointeur en objet Rust
    let mon_deencap = Box::new(Decapsulator::new(
        decap_buffer,
        DefaultCrc {},
        SimpleMandatoryExtensionHeaderManager {},
    ));
    Box::into_raw(mon_deencap) // Renvoyer l'objet (ou None si nécessaire)
}

#[no_mangle]
pub extern "C" fn rust_decap(c_buffer: RustSlice, ptr: &mut OpaquePtrDecap) -> RustDecapStatus {
    let rs_buffer = c_buffer.to_slice();

    let decapsul = &mut *ptr;

    match decapsul.decap(rs_buffer) {
        Ok((rs_decap_status, len_pkt)) => {
            new_decap_status_from_rust_status(rs_decap_status, len_pkt)
        }
        Err((rs_decap_error, len_pkt)) => new_decap_status_from_rust_error(rs_decap_error, len_pkt),
    }
}

#[repr(C)]
pub enum RustExtractLabelorFragIdType {
    ResLbl,
    ResFragId,
    ErrorLabelReUse,
    ErrorSizeBuffer,
    ErrorHeaderRead,
    ErrorUnkownMandatoryHeader,
}

type FragId = u8;

#[repr(C)]
pub union RustExtractLabelorFragIdValue {
    fragid: FragId,
    label: RustLabel,
    other: u8,
}

#[repr(C)]

pub struct RustExtractLabelorFragIdStatus {
    status: RustExtractLabelorFragIdType,
    value: RustExtractLabelorFragIdValue,
}
use dvb_gse_rust::header_extension::SimpleMandatoryExtensionHeaderManager;

#[repr(C)]
pub enum RustGetExtensionsHeaderType {
    NoExtensionsHeader,
    ExtensionsHeader,
    ErrNoextension,
    ErrReadingPacket,
    ErrBufferTooSmall,
    ErrNotGSEPacket,
}

#[repr(C)]
pub struct RustGetExtensionsHeader {
    status: RustGetExtensionsHeaderType,
    value: CHeaderExtensionSlice,
}

fn new_fragorlbl_stat_from_rust_status(res: LabelorFragId) -> RustExtractLabelorFragIdStatus {
    match res {
        LabelorFragId::FragId(id) => RustExtractLabelorFragIdStatus {
            status: RustExtractLabelorFragIdType::ResFragId,
            value: RustExtractLabelorFragIdValue { fragid: id },
        },
        LabelorFragId::Lbl(l) => RustExtractLabelorFragIdStatus {
            status: RustExtractLabelorFragIdType::ResLbl,
            value: RustExtractLabelorFragIdValue {
                label: new_label_fromrs(l),
            },
        },
    }
}

fn new_fragorlbl_stat_from_rust_error(
    err: GetLabelorFragIdError,
) -> RustExtractLabelorFragIdStatus {
    match err {
        GetLabelorFragIdError::ErrLabelReuse => RustExtractLabelorFragIdStatus {
            status: RustExtractLabelorFragIdType::ErrorLabelReUse,
            value: RustExtractLabelorFragIdValue { other: 0 },
        },
        GetLabelorFragIdError::ErrSizeBuffer => RustExtractLabelorFragIdStatus {
            status: RustExtractLabelorFragIdType::ErrorSizeBuffer,
            value: RustExtractLabelorFragIdValue { other: 0 },
        },

        GetLabelorFragIdError::ErrHeaderRead => RustExtractLabelorFragIdStatus {
            status: RustExtractLabelorFragIdType::ErrorHeaderRead,
            value: RustExtractLabelorFragIdValue { other: 0 },
        },
        GetLabelorFragIdError::ErrorUnkownMandatoryHeader => RustExtractLabelorFragIdStatus {
            status: RustExtractLabelorFragIdType::ErrorUnkownMandatoryHeader,
            value: RustExtractLabelorFragIdValue { other: 0 },
        },
    }
}

#[no_mangle]
pub extern "C" fn rust_getFragIdOrLbl(
    c_buffer: RustSlice,
    ptr: &mut OpaquePtrDecap,
) -> RustExtractLabelorFragIdStatus {
    let rs_pdu: &[u8] = c_buffer.to_slice();
    let decapsul = &mut *ptr;

    match decapsul.get_label_or_frag_id(rs_pdu) {
        Ok(r) => new_fragorlbl_stat_from_rust_status(r),
        Err(e) => new_fragorlbl_stat_from_rust_error(e),
    }
}

// #[no_mangle]
// pub extern "C" fn rust_getExtensionHeaders(
//     c_buffer: RustSlice,
//     ptr: &mut OpaquePtrDecap
// ) -> RustGetExtensionsHeader {
//     let rs_pdu: &[u8] = c_buffer.to_slice();
//     let decapsul = &mut *ptr;

//     match decapsul.get_extensions(rs_pdu) {
//         Err(e) => match e {
//             ExtensionError::Noextension => RustGetExtensionsHeader {
//                 status: RustGetExtensionsHeaderType::ErrNoextension,
//                 value:  CHeaderExtensionSlice {
//                     size: 0,
//                     bytes: std::ptr::null()}},

//             ExtensionError::ErrReadingPacket => RustGetExtensionsHeader {
//                 status: RustGetExtensionsHeaderType::ErrReadingPacket,
//                 value:  CHeaderExtensionSlice {
//                     size: 0,
//                     bytes: std::ptr::null()}},

//             ExtensionError::ErrBufferTooSmall => RustGetExtensionsHeader {
//                 status: RustGetExtensionsHeaderType::ErrBufferTooSmall,
//                 value:  CHeaderExtensionSlice {
//                     size: 0,
//                     bytes: std::ptr::null()}},

//             ExtensionError::ErrNotGSEPacket => RustGetExtensionsHeader {
//                 status: RustGetExtensionsHeaderType::ErrNotGSEPacket,
//                 value:  CHeaderExtensionSlice {
//                 size: 0,
//                 bytes: std::ptr::null()}},
//         },

//         Ok(res) => match res {
//                 None => RustGetExtensionsHeader {
//                     status: RustGetExtensionsHeaderType::NoExtensionsHeader,
//                     value:  CHeaderExtensionSlice {
//                         size: 0,
//                         bytes: std::ptr::null()},},

//                 Some(val) => {
//                     RustGetExtensionsHeader {
//                     status: RustGetExtensionsHeaderType::ExtensionsHeader,
//                     value:  CHeaderExtensionSlice::from(from_rust_vec_to_c_vec(val))}
//                 },
//             }

//     }
// }
