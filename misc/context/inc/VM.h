#ifndef VM_CLASS
#define VM_CLASS

#ifndef __cplusplus
    extern "C" {
#endif

#include "mach.h"
#include "vm_conf.h"
    
INT32_T VM_SYS_THREAD (arch_word_t, void *);
        
enum {
    VM_OK,
    VM_SMALL_CORE,
    VM_CREATE_ERR,
    VM_DEPRECATED_CALL,
    VM_UNKNOWN_CALL,
    VM_NOT_FOUND,
    VM_DUP_CALL,
};

#ifndef __cplusplus
    }
#endif        
        
        
#endif


/*End of file*/

