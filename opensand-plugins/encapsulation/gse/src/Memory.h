#include "GseRustCApi.h"


extern "C" {
// Allocate and initialise c_memory fields
c_memory c_memory_new(size_t max_frag_id, size_t max_pdu_size);

// Deallocate c_memory fields
void c_memory_delete(c_memory memory);

// Provision of a storage buffer
bool c_memory_provision_storage(c_memory *memory, RustMutSlice storage);

// Get a buffer_pdu from storage
RustMutSlice c_memory_new_pdu(c_memory *memory);

// Create a MemoryContext from storage or get a MemoryContext from frags
RustMemoryContext c_memory_new_frag(c_memory *memory, RustDecapContext context);

// Get a MemoryContext from frags
RustMemoryContext c_memory_take_frag(struct c_memory *memory, uint8_t frag_id);

// Save a Memorycontext into frags
void c_memory_save_frag(c_memory *memory, RustMemoryContext context);
}
