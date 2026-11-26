#ifndef VUESP_CONST_BYTECODE_H
#define VUESP_CONST_BYTECODE_H

#ifndef ENABLE_BYTECODE_VM
#define ENABLE_BYTECODE_VM 0
#endif

// Number of bytecode containers kept in RAM. Override per build/board, e.g.
// -D VBC_CACHE_SLOTS=4. One slot is 1 KB (VBC_MAX_CODE_BYTES).
#ifndef VBC_CACHE_SLOTS
#define VBC_CACHE_SLOTS 2
#endif

#endif
