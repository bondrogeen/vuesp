#ifndef VUESP_CONST_BYTECODE_H
#define VUESP_CONST_BYTECODE_H

// Number of bytecode containers kept in RAM. Override per build/board, e.g.
// -D VBC_CACHE_SLOTS=2. One slot is 1 KB (VBC_MAX_CODE_BYTES). The default 3
// lets boot call two other scripts without evicting an active context.
#ifndef VBC_CACHE_SLOTS
#define VBC_CACHE_SLOTS 3
#endif

#endif
