#ifndef THREAD_TYPES_H
#define THREAD_TYPES_H

#include <stdint.h>
#include "mach.h"

#ifndef     THREAD_MAX_COUNT    
#define     THREAD_MAX_COUNT (24U)
#endif

#ifndef     VM_THREAD_MAX_PRIORITY  
#define     VM_THREAD_MAX_PRIORITY  (8U)
#endif
        
#ifndef     VM_DEF_THREAD_HEAP_SIZE
#define     VM_DEF_THREAD_HEAP_SIZE (6144U)
#endif
        
#ifndef     VM_MIN_THREAD_HEAP_SIZE
#define     VM_MIN_THREAD_HEAP_SIZE (FPU_STACK_SIZE + CPU_STACK_SIZE + 128)
#endif
        
#ifndef     VM_DEF_THREAD_NAME_LEN
#define     VM_DEF_THREAD_NAME_LEN (24U)
#endif

#define     IDLE_THREAD_ID  (0U)    

typedef V_PREPACK struct {
    void *t_entry_point;
    arch_word_t StackSize;
    arch_word_t argSize;
    void *Arg;
    const char *Name;
    arch_byte_t Priority;
} vthread_t;
       
typedef INT32_T (*vm_thread_event_t) (arch_word_t type, void *link);
        
enum {
    THREAD_STOP = 0x0U,
    THREAD_RUN  = 0x1U,
    THREAD_PEND = 0x2U,
};

enum {
    THREAD_NO_FPU = 0x0U,
    THREAD_FPU = 0x1U,
};

enum {
    T_OK,
    T_LONG_NAME,
    T_SMALL_HEAP,
    T_SMALL_CORE,
    T_NULL_CALLBACK,
    T_PRIV_UNDEF,
    T_PRIOR_UNDEF,
};

#define VM_THREAD_DESC(ptr) (struct vm_thread_desc *)(ptr)
    
#define VM_THREAD_LIST_FIRST(list) (struct vm_thread_desc *)(list)->firstLink

#define VM_THREAD_LIST_LAST(list) (struct vm_thread_desc *)(list)->lastLink

#define VM_THREAD_LIST_ELEMS(list) (list)->elements

#define VM_THREAD_LNODE_DEF() \
    V_PREPACK struct { struct vm_thread_desc *nextLink ,*prevLink; }

#define VM_THREAD_LHEAD_INIT(list) \
do { \
    (list)->elements = 0; \
} while (0)

/*
#define VM_THREAD_LHEAD_INIT(list) \
do { \
    VM_THREAD_LIST_FIRST(list) = NULL; \
    VM_THREAD_LIST_LAST(list) = NULL; \
    VM_THREAD_LIST_ELEMS(list) = 0; \
} while (0)
*/

#define VM_THREAD_LFOREACH_SAFE(list, t, tn) \
for (struct vm_thread_desc *t = VM_THREAD_LIST_FIRST(list), \
     *tn = t->nextLink; \
     t != NULL; t = tn, tn = tn->nextLink )


typedef V_PREPACK struct {
    void *firstLink, *lastLink;
    arch_word_t elements;
} vm_thread_list_t;

V_PREPACK struct vm_thread_desc {
    CPU_STACK_FRAME *CPU_FRAME;
    arch_word_t tdesc_wait_cnt;
    arch_word_t tdesc_stack_floor;
    arch_word_t tdesc_stack_size;
    VM_THREAD_LNODE_DEF();
    const char *faultMessage;
    V_PREPACK union {
        void *link;
        vm_thread_event_t cond;
        const char *event_name;
    };
    void *caller;
    vm_thread_list_t chain;
    INT64_T cpuUsage;
    INT32_T  ID;
    arch_byte_t PRIORITY;
    arch_byte_t V_PRIORITY;
    arch_byte_t PRIVILEGE;
    char name[VM_DEF_THREAD_NAME_LEN];
#ifdef __LITTLE_ENDIAN_BF__
    unsigned STATUS : 3;
    unsigned USE_FPU : 1;
    unsigned mutex : 1;
    unsigned monitor : 1;
    unsigned fault : 1;
    unsigned waitNotify : 1;
#elif defined(__BIG_ENDIAN_BF__)
    unsigned waitNotify : 1;
    unsigned fault : 1;
    unsigned monitor : 1;
    unsigned mutex : 1;
    unsigned USE_FPU : 1;
    unsigned STATUS : 3;  
#else
   #error "endiannes."
#endif
};

#endif /*THREAD_TYPES_H*/
