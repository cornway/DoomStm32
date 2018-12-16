#ifndef VMAPI_INTERFACE
#define VMAPI_INTERFACE

#include "arch.h"
#include "thread_types.h"
#include "mail_types.h"
#include "drv_types.h"
#include "vmapi_call.h"
#include <stdlib.h>

#define STATIC_ASSERT(x, msg) static_assert(x, msg)

extern "C" _VALUES_IN_REGS ARG_STRUCT_T upc (ARG_STRUCT_T a);

_WEAK INT32_T VMAPI_ErrorHandler (arch_word_t from, _VALUES_IN_REGS ARG_STRUCT_T arg);

#define dispatch_from_svc(name, ...) \
ARG_STRUCT_T name = {__VA_ARGS__}; \
do { \
    name.IRQ = VM_CALL_FROM_IRQ; \
    name = arch_upcall_alias(name); \
} while (0)


namespace vm {

_VALUES_IN_REGS ARG_STRUCT_T reset ();
_VALUES_IN_REGS ARG_STRUCT_T init ();
_VALUES_IN_REGS ARG_STRUCT_T start (); 
_VALUES_IN_REGS ARG_STRUCT_T restart (); 
    
_VALUES_IN_REGS ARG_STRUCT_T sleep (UINT32_T delay);
_VALUES_IN_REGS ARG_STRUCT_T yield (void);
_VALUES_IN_REGS ARG_STRUCT_T create (vthread_t *th);
_VALUES_IN_REGS ARG_STRUCT_T create (v_callback_t callback, const char *name, arch_word_t stack, arch_word_t prio, arch_word_t size, void *arg);
_VALUES_IN_REGS ARG_STRUCT_T call (v_callback_t callback, const char *name, arch_word_t stack, arch_word_t prio, arch_word_t size, void *arg);
_VALUES_IN_REGS ARG_STRUCT_T drv_link (drv_handle_t *handler, uint32_t irq, uint32_t dma);
_VALUES_IN_REGS ARG_STRUCT_T drv_unlink (uint32_t id);
_VALUES_IN_REGS ARG_STRUCT_T drv_ctl (uint32_t id, uint32_t ctl0, uint32_t ctl1);
_VALUES_IN_REGS ARG_STRUCT_T drv_io (uint32_t id, drv_data_t *data);
_VALUES_IN_REGS ARG_STRUCT_T drv_probe (const char *name);
_VALUES_IN_REGS ARG_STRUCT_T lock (UINT32_T id);
_VALUES_IN_REGS ARG_STRUCT_T unlock (UINT32_T id);
_VALUES_IN_REGS ARG_STRUCT_T notify (const char *name);   
_VALUES_IN_REGS ARG_STRUCT_T wait_notify ();  
_VALUES_IN_REGS ARG_STRUCT_T notify_wait (const char *name);   
_VALUES_IN_REGS ARG_STRUCT_T sync (const char *name);       
_VALUES_IN_REGS ARG_STRUCT_T wait (vm_thread_event_t cond);
_VALUES_IN_REGS ARG_STRUCT_T wait_event (const char *event_name);
_VALUES_IN_REGS ARG_STRUCT_T fire_event (const char *event_name);   
_VALUES_IN_REGS ARG_STRUCT_T mail (char *name, MAIL_HANDLE *mail);
_VALUES_IN_REGS ARG_STRUCT_T wait_mail ();
_VALUES_IN_REGS ARG_STRUCT_T timer (arch_word_t *dest, arch_word_t id);
_VALUES_IN_REGS ARG_STRUCT_T timer_remove (arch_word_t id);

_VALUES_IN_REGS ARG_STRUCT_T critical ();
_VALUES_IN_REGS ARG_STRUCT_T end_critical ();
_VALUES_IN_REGS ARG_STRUCT_T exit (UINT32_T ret);
_VALUES_IN_REGS ARG_STRUCT_T fault (const char *cause);
  
class Mutex {
    private :
        uint32_t id;
     public :
        Mutex (uint32_t _id)
        {
            id = _id;
            lock(id);
        }
        Mutex (void  *_id)
        {
            id = (uint32_t)_id;
            lock(id);
        }
        ~ Mutex ()
        {
            unlock(id);
        }

        bool unlock__ ()
        {
            unlock(id);
            return true;
        }
};

class Monitor4 {
    private :
        uint32_t id[4];
    public :
        Monitor4 (uint32_t  a, uint32_t  b, uint32_t  c, uint32_t  d) {
            id[0] = a;
            id[1] = b;
            id[2] = c;
            id[3] = d;
            if (a) lock(a);
            if (b) lock(b);
            if (c) lock(c);
            if (d) lock(d);
        }

        ~Monitor4 ()
        {
            if (id[0]) unlock(id[0]);
            if (id[1]) unlock(id[1]);
            if (id[2]) unlock(id[2]);
            if (id[3]) unlock(id[3]);
        }
};

#define MON4(args ...) \
 do { \
    uint32_t args_[4] = {args}; \
    Monitor4(args_[0], args_[1], args_[2], args_[3]); \
} while (0)

class Cleanup {
    private :
        void *p;
    public :
        Cleanup (void *_p)
            {
                p = _p;
            }
       ~Cleanup ()
        {
                free(p);
        }
};

};


#define _XCALL(ret, callback, size, arg) \
ARG_STRUCT_T _UNUSED(ret); \
do { \
    ret = vm::call(callback, #callback, VM_DEF_THREAD_HEAP_SIZE, VM_THREAD_DEF_PRIORITY, size, arg); \
} while (0)

#define __XCALL(ret, stack, callback, size, arg) \
ARG_STRUCT_T _UNUSED(ret); \
do { \
    ret = vm::call(callback, #callback, stack, VM_THREAD_DEF_PRIORITY, size, arg); \
} while (0)

#define __XCRE(ret, stack, callback, size, arg) \
ARG_STRUCT_T _UNUSED(ret); \
do { \
    ret = vm::create(callback, #callback, stack, VM_THREAD_DEF_PRIORITY, size, arg); \
} while (0)

#define VM_SLEEP(n) \
for (int i = 0; i < n; i++) { \
    vm::yield(); \
}

#endif


/*End of file*/


