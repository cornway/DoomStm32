#ifndef VM_THREAD
#define VM_THREAD

#ifdef __cplusplus
    extern "C" {
#endif

#include "stdint.h"
#include "vm_conf.h"
#include "arch.h"
#include "thread_types.h"

_VALUES_IN_REGS ARG_STRUCT_T VMBREAK (UINT32_T ret);

void t_init_core (void);
void t_destroy (struct vm_thread_desc *t);
int t_init (struct vm_thread_desc **t, 
                        v_callback_t callback, 
                        arch_byte_t priority, 
                        arch_word_t id, 
                        arch_word_t heap_size, 
                        arch_byte_t privilege, 
                        const char *name, 
                        arch_word_t arg_size, 
                        void *arg,
                        arch_byte_t is_idle);
struct vm_thread_desc *t_ready (UINT32_T priority);
struct vm_thread_desc *t_search (const char *name);
void t_unlink_ready (struct vm_thread_desc *t);
void t_link_chain (struct vm_thread_desc *t, struct vm_thread_desc *l);
void t_unchain (void (*linker) (struct vm_thread_desc *), struct vm_thread_desc *t);
void t_link_ready (struct vm_thread_desc *t);
void t_link_drop (struct vm_thread_desc *t);
void t_link_delay (struct vm_thread_desc *t);
void t_link_fault (struct vm_thread_desc *t);
void t_link_notify (struct vm_thread_desc *t);
void t_unlink_notify (struct vm_thread_desc *t);
void t_link_mail (struct vm_thread_desc *t);
void t_unlink_mail(struct vm_thread_desc *t);
struct vm_thread_desc *t_mail (const char *name);
void t_link_cond (struct vm_thread_desc *t);
arch_word_t t_check_cond (void (*linker) (struct vm_thread_desc *));
void t_link_wait_event (struct vm_thread_desc *t);
arch_word_t t_fire_event (void (*linker) (struct vm_thread_desc *), const char *event_name);
struct vm_thread_desc *t_notify (const char *name);
int t_check_list (void);
void t_refresh (void (*linker) (struct vm_thread_desc *));
void t_tick (void (*linker) (struct vm_thread_desc *));
        
#ifdef __cplusplus
    }
#endif

#endif /*VM_THREAD*/


/*End of file*/

