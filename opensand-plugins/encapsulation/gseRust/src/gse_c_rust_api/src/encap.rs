use crate::common::{RustLabel, RustMutSlice, RustSlice};
use crate::header_ext::{from_c_vec_to_rust_vec, CHeaderExtensionSlice};
use dvb_gse_rust::crc::DefaultCrc;
use dvb_gse_rust::gse_encap::{ContextFrag, EncapError, EncapMetadata, EncapStatus, Encapsulator};

#[repr(C)]
#[derive(Clone, Copy)]
pub struct RustContextFrag {
    frag_id: u8,
    crc: u32,
    len_pdu_frag: u16,
}

pub type OpaquePtrEncap = Encapsulator<DefaultCrc>;

#[repr(C)]
pub struct RustEncapStatus {
    status: RustEncapStatusType,
    value: RustEncapStatusValue,
}

#[repr(C)]
#[derive(Copy, Clone)]
pub struct EncapStatusFrag {
    len_pkt: u16,
    context: RustContextFrag,
}
#[repr(C)]
#[derive(Copy, Clone)]
pub union RustEncapStatusValue {
    completed_pkt: u16,
    fragmented_pkt: EncapStatusFrag,
    other: u8,
}
#[repr(C)]

pub struct RustEncapMetadata {
    protocol_type: u16,
    label: RustLabel,
}

#[repr(C)]
pub enum RustEncapStatusType {
    EncapCompletedPkt,
    EncapFragmentedPkt,
    EncapErrorPduLength,
    EncapErrorSizeBuffer,
    EncapErrorSizePduBuffer,
    EncapErrorProtocolType,
    EncapErrorInvalidLabel,
    EncapErrorFinalMandatoryExtensionHeader,
}

fn new_encap_metadata_fromc(c_metadata: RustEncapMetadata) -> EncapMetadata {
    EncapMetadata {
        protocol_type: c_metadata.protocol_type,

        label: c_metadata.label.to_rust_label(),
    }
}

fn new_rust_context_frag(rs_context: ContextFrag) -> RustContextFrag {
    RustContextFrag {
        frag_id: rs_context.frag_id,
        crc: rs_context.crc,
        len_pdu_frag: rs_context.len_pdu_frag,
    }
}

fn new_encap_status_from_rust_status(rs_status: EncapStatus) -> RustEncapStatus {
    let status: RustEncapStatus = match rs_status {
        EncapStatus::CompletedPkt(len_pkt) => RustEncapStatus {
            status: RustEncapStatusType::EncapCompletedPkt,
            value: RustEncapStatusValue {
                completed_pkt: len_pkt,
            },
        },
        EncapStatus::FragmentedPkt(len_pkt, rs_context) => {
            let context = new_rust_context_frag(rs_context);
            RustEncapStatus {
                status: RustEncapStatusType::EncapFragmentedPkt,
                value: RustEncapStatusValue {
                    fragmented_pkt: EncapStatusFrag { len_pkt, context },
                },
            }
        }
    };
    status
}

fn new_encap_status_from_rust_error(rs_error: EncapError) -> RustEncapStatus {
    let status: RustEncapStatus = match rs_error {
        EncapError::ErrorInvalidLabel => RustEncapStatus {
            value: RustEncapStatusValue { other: 0 },
            status: RustEncapStatusType::EncapErrorInvalidLabel,
        },
        EncapError::ErrorPduLength => RustEncapStatus {
            value: RustEncapStatusValue { other: 0 },
            status: RustEncapStatusType::EncapErrorPduLength,
        },
        EncapError::ErrorProtocolType => RustEncapStatus {
            value: RustEncapStatusValue { other: 0 },
            status: RustEncapStatusType::EncapErrorProtocolType,
        },
        EncapError::ErrorSizeBuffer => RustEncapStatus {
            value: RustEncapStatusValue { other: 0 },
            status: RustEncapStatusType::EncapErrorSizeBuffer,
        },
        EncapError::ErrorNoExtensionFound => todo!(),
    
    EncapError::ErrorFinalMandatoryExtensionHeader => RustEncapStatus {
        value: RustEncapStatusValue { other: 0 },
        status: RustEncapStatusType::EncapErrorFinalMandatoryExtensionHeader,
    },
};
    status
}

pub fn get_encapsulator_from_ptr(ptr: &mut OpaquePtrEncap) -> Option<&Encapsulator<DefaultCrc>> {
    // Convertir le pointeur en objet Rust
    let encap = &mut *ptr;

    Some(encap)
}


/// # Safety
/// ## Safety Guarantees
/// the encapsulator should be free using delete_encapsulator
#[no_mangle]
pub extern "C" fn create_encapsulator() -> *mut OpaquePtrEncap {
    // Convertir le pointeur en objet Rust
    let mon_encap = Box::new(Encapsulator::new(DefaultCrc {}));
    Box::into_raw(mon_encap) // Renvoyer l'objet (ou None si nécessaire)
}


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
#[no_mangle]
pub unsafe extern "C" fn delete_encapsulator(ptr: *mut OpaquePtrEncap) {
    unsafe { drop(Box::from_raw(ptr)) }
}


impl RustContextFrag {
    fn to_context_frag(self) -> ContextFrag {
        ContextFrag {
            frag_id: self.frag_id,
            crc: self.crc,
            len_pdu_frag: self.len_pdu_frag,
        }
    }
}

#[no_mangle]
pub extern "C" fn enable_labelReUse(ptr: &mut OpaquePtrEncap, enable: bool){
    let encapsul = &mut *ptr;
    encapsul.enable_re_use_label(enable);
}

#[no_mangle]
pub extern "C" fn rust_encap_frag(
    c_pdu: RustSlice,
    c_context: RustContextFrag,
    buffer: RustMutSlice,
    ptr: &mut OpaquePtrEncap,
) -> RustEncapStatus {
    let rs_pdu = c_pdu.to_slice();
    let rs_context: ContextFrag = c_context.to_context_frag();
    let rs_buffer: &mut [u8] = buffer.to_slice();
    let encapsul: &mut Encapsulator<DefaultCrc> = &mut *ptr;
    match encapsul.encap_frag(rs_pdu, &rs_context, rs_buffer) {
        Ok(rust_encap_status) => new_encap_status_from_rust_status(rust_encap_status),
        Err(rust_error_status) => new_encap_status_from_rust_error(rust_error_status),
    }
}


#[no_mangle]
pub extern "C" fn rust_encap(
    c_pdu: RustSlice,
    frag_id: u8,
    c_metadata: RustEncapMetadata,
    c_buffer: RustMutSlice,
    ptr: &mut OpaquePtrEncap,
) -> RustEncapStatus {
    let rs_pdu: &[u8] = c_pdu.to_slice();

    let rs_metadata: EncapMetadata = new_encap_metadata_fromc(c_metadata);

    let rs_buffer: &mut [u8] = c_buffer.to_slice();

    let encapsul: &mut Encapsulator<DefaultCrc> = &mut *ptr;

    match encapsul.encap(rs_pdu, frag_id, rs_metadata, rs_buffer) {
        Ok(rust_encap_status) => new_encap_status_from_rust_status(rust_encap_status),
        Err(rust_error_status) => new_encap_status_from_rust_error(rust_error_status),
    }
}



#[no_mangle]
pub extern "C" fn rust_encap_ext(
    c_pdu: RustSlice,
    frag_id: u8,
    c_metadata: RustEncapMetadata,
    c_buffer: RustMutSlice,
    ptr: &mut OpaquePtrEncap,
    ext: CHeaderExtensionSlice
) -> RustEncapStatus {
    let ext2: Vec<dvb_gse_rust::header_extension::Extension> = from_c_vec_to_rust_vec(ext.to_slice());
    let rs_pdu: &[u8] = c_pdu.to_slice();
    let rs_metadata: EncapMetadata = new_encap_metadata_fromc(c_metadata);
    let rs_buffer: &mut [u8] = c_buffer.to_slice();
    let encapsul: &mut Encapsulator<DefaultCrc> = &mut *ptr;
    match encapsul.encap_ext(rs_pdu, frag_id, rs_metadata, rs_buffer, ext2) {
        Ok(rust_encap_status) => new_encap_status_from_rust_status(rust_encap_status),
        Err(rust_error_status) => new_encap_status_from_rust_error(rust_error_status),
    }
}