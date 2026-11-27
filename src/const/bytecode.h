#ifndef VUESP_CONST_BYTECODE_H
#define VUESP_CONST_BYTECODE_H

#ifndef ENABLE_BYTECODE_VM
#define ENABLE_BYTECODE_VM 0
#endif

// Number of bytecode containers kept in RAM. Override per build/board, e.g.
// -D VBC_CACHE_SLOTS=2. One slot is 1 KB (VBC_MAX_CODE_BYTES). The default 3
// lets boot call two other scripts without evicting an active context.
#ifndef VBC_CACHE_SLOTS
#define VBC_CACHE_SLOTS 3
#endif

// The legacy text runner keeps its full slot table in normal builds. The
// bytecode build only needs a few slots for the old pages, so it defaults to 4
// to leave DRAM for the VM and Wi-Fi stack.
#ifndef VBC_LEGACY_SCRIPT_SLOTS
#if ENABLE_BYTECODE_VM
#define VBC_LEGACY_SCRIPT_SLOTS 4
#else
#define VBC_LEGACY_SCRIPT_SLOTS 15
#endif
#endif

#endif
