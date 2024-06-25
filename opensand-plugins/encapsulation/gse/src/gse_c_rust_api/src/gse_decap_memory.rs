use dvb_gse_rust::gse_decap::{
    gse_decap_memory::MemoryContext, DecapContext, DecapMemoryError, GseDecapMemory,
};
use libc::size_t;
use std::mem::ManuallyDrop;

use crate::{common::{cslice_from_rsslice, RustMutSlice}, decap::{new_decap_context_fromrs, new_memory_context_fromrs, RustDecapContext, RustMemoryContext}};

// Import C function to convert them in Rust trait
#[allow(unused_doc_comments)]
/// cbindgen:ignore
extern "C" {
    #[allow(improper_ctypes, unused)]
    pub fn c_memory_new(max_frag_id: size_t, max_pdu_size: size_t) -> c_memory;

    #[allow(improper_ctypes, unused)]
    pub fn c_memory_provision_storage(memory: &mut c_memory, storage: RustMutSlice) -> bool;

    #[allow(improper_ctypes, unused)]
    pub fn c_memory_new_pdu(memory: &mut c_memory) -> RustMutSlice;

    #[allow(improper_ctypes, unused)]
    pub fn c_memory_new_frag(memory: &mut c_memory, context: RustDecapContext)
        -> RustMemoryContext;

    #[allow(improper_ctypes, unused)]
    pub fn c_memory_take_frag(memory: &mut c_memory, frag_id: u8) -> RustMemoryContext;

    #[allow(improper_ctypes, unused)]
    pub fn c_memory_save_frag(memory: &mut c_memory, context: RustMemoryContext);
}

#[repr(C)]
#[allow(non_camel_case_types, improper_ctypes, unused)]
pub struct c_memory {
    frags: *mut RustMemoryContext,
    storage: *mut RustMutSlice,
    max_frag_id: size_t,
    max_pdu_size: size_t,
}

impl GseDecapMemory for c_memory {
    fn new(
        max_frag_id: usize,
        max_pdu_size: usize,
        _max_delay: usize,
        _max_pdu_frag: usize,
    ) -> Self {
        unsafe { c_memory_new(max_frag_id, max_pdu_size) }
    }

    fn provision_storage(&mut self, storage: Box<[u8]>) -> Result<(), DecapMemoryError> {
        let mut storage = ManuallyDrop::new(storage);
        let slice_storage = cslice_from_rsslice(&mut storage);

        if unsafe { c_memory_provision_storage(self, slice_storage) } {
            return Ok(());
        };

        Err(DecapMemoryError::MemoryCorrupted)
    }

    fn new_pdu(&mut self) -> Result<Box<[u8]>, DecapMemoryError> {
        let pdu_buffer = unsafe { c_memory_new_pdu(self) };

        if pdu_buffer.bytes.is_null() {
            return Err(DecapMemoryError::StorageUnderflow);
        };

        Ok(pdu_buffer.to_box())
    }

    fn new_frag(&mut self, context: DecapContext) -> Result<MemoryContext, DecapMemoryError> {
        let c_context = new_decap_context_fromrs(context);
        let c_memory_context = unsafe { c_memory_new_frag(self, c_context) };

        if c_memory_context.pdu.bytes.is_null() {
            return Err(DecapMemoryError::StorageUnderflow);
        }

        Ok(c_memory_context.to_rust_memory_context())
    }

    fn take_frag(&mut self, frag_id: u8) -> Result<MemoryContext, DecapMemoryError> {
        let c_memory_context = unsafe { c_memory_take_frag(self, frag_id) };

        if c_memory_context.pdu.bytes.is_null() {
            return Err(DecapMemoryError::UndefinedId);
        }

        Ok(c_memory_context.to_rust_memory_context())
    }

    fn save_frag(&mut self, context: MemoryContext) -> Result<(), DecapMemoryError> {
        let c_context = new_memory_context_fromrs(context);
        unsafe { c_memory_save_frag(self, c_context) };
        Ok(())
    }
}
