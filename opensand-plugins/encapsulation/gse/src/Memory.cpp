#include "Memory.h"


// Allocate and initialise c_memory fields
c_memory c_memory_new(size_t max_frag_id, size_t max_pdu_size) {

  RustMemoryContext *frags =
      (RustMemoryContext *)calloc(max_frag_id, sizeof(RustMemoryContext));

  RustMutSlice *storage =
      (RustMutSlice *)calloc(max_frag_id + 2, sizeof(RustMutSlice));

  for (size_t i = 0; i < max_frag_id + 2; i++) {
    storage[i].bytes = (uint8_t *)calloc(max_pdu_size, sizeof(uint8_t));
    storage[i].size = max_pdu_size;
  }

  return (c_memory){
      .frags = frags,
      .storage = storage,
      .max_frag_id = max_frag_id,
      .max_pdu_size = max_pdu_size,
  };
}

// Deallocate c_memory fields
void c_memory_delete(c_memory memory) {

  for (size_t i = 0; i < memory.max_frag_id; i++) {
    if (memory.frags[i].pdu.bytes != NULL) {
      free(memory.frags[i].pdu.bytes);
    }
    if (memory.frags[i].pdu.bytes != NULL) {
      free(memory.storage[i].bytes);
    }
  }

  free(memory.frags);
  free(memory.storage);
}

// Provision of a storage buffer
bool c_memory_provision_storage(c_memory *memory, RustMutSlice storage) {

  for (int i = memory->max_frag_id - 1; i >= 0; i--) {
    if (memory->storage[i].bytes == NULL &&
        memory->max_pdu_size <= storage.size) {
      memory->storage[i] = storage;
      return true;
    }
  }
  return false;
}

// Get a buffer_pdu from storage
RustMutSlice c_memory_new_pdu(c_memory *memory) {

  for (uint8_t i = 0; i < memory->max_frag_id; i++) {
    if (memory->storage[i].bytes != NULL) {
      RustMutSlice flag = memory->storage[i];
      memory->storage[i] = (RustMutSlice){.size = 0, .bytes = NULL};
      return flag;
    }
  }
  return (RustMutSlice){.size = 0, .bytes = NULL};
}

// Create a MemoryContext from storage or get a MemoryContext from frags
RustMemoryContext c_memory_new_frag(c_memory *memory,
                                    RustDecapContext context) {
  uint8_t idx = context.frag_id % memory->max_frag_id;

  if (memory->frags[idx].pdu.bytes == NULL) {
    return (RustMemoryContext){.context = context,
                               .pdu = c_memory_new_pdu(memory)};
  }

  RustMutSlice flag = memory->frags[idx].pdu;
  memory->frags[idx].pdu = (RustMutSlice){.size = 0, .bytes = NULL};
  return (RustMemoryContext){.context = context, .pdu = flag};
}

// Get a MemoryContext from frags
RustMemoryContext c_memory_take_frag  (struct c_memory *memory, uint8_t frag_id) {
  uint8_t idx = frag_id % memory->max_frag_id;
  RustMemoryContext flag = memory->frags[idx];

  memory->frags[idx].pdu = (RustMutSlice){.size = 0, .bytes = NULL};
  return flag;
}

// Save a Memorycontext into frags
void c_memory_save_frag(c_memory *memory, RustMemoryContext mem_context) {
  uint8_t idx = mem_context.context.frag_id % memory->max_frag_id;
  memory->frags[idx] = mem_context;
}
