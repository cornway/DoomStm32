
#include "vmapi.h"

#define upc_(...) \
do { \
    ARG_STRUCT_T arg = {__VA_ARGS__}; \
    return upc(arg); \
} while (0)


_VALUES_IN_REGS ARG_STRUCT_T vm::restart ()
{
    upc_();
}

_VALUES_IN_REGS ARG_STRUCT_T vm::sleep (UINT32_T delay)
{
    upc_(VMAPI_SLEEP, delay);
}

_VALUES_IN_REGS ARG_STRUCT_T vm::yield (void)
{
    upc_(VMAPI_YIELD);
}

_VALUES_IN_REGS ARG_STRUCT_T vm::create (vthread_t *th)
{
    upc_(VMAPI_CREATE, (arch_word_t)th);
}

_VALUES_IN_REGS ARG_STRUCT_T vm::create (v_callback_t callback, const char *name, arch_word_t stack, arch_word_t prio, arch_word_t size, void *arg)
{
    vthread_t th;
    th.t_entry_point = (void *)callback;
    th.Name = name;
    th.Priority = prio;
    th.StackSize = stack;
    th.argSize = size;
    th.Arg = arg;
    return vm::create(&th);
}

_VALUES_IN_REGS ARG_STRUCT_T vm::call (v_callback_t callback, const char *name, arch_word_t stack, arch_word_t prio, arch_word_t size, void *arg)
{
    vthread_t th;
    th.t_entry_point = (void *)callback;
    th.Name = name;
    th.Priority = prio;
    th.StackSize = stack;
    th.argSize = size;
    th.Arg = arg;
    upc_(VMAPI_CALL, (arch_word_t)&th);
}

_VALUES_IN_REGS ARG_STRUCT_T vm::drv_link (drv_handle_t *handler, uint32_t irq, uint32_t dma)
{
    upc_(VMAPI_DRV_ATTACH, (arch_word_t)handler, irq, dma);
}

_VALUES_IN_REGS ARG_STRUCT_T vm::drv_unlink (uint32_t id)
{
    upc_(VMAPI_DRV_DETTACH, id);
}

_VALUES_IN_REGS ARG_STRUCT_T vm::drv_ctl (uint32_t id, uint32_t ctl0, uint32_t ctl1)
{
    upc_(VMAPI_DRV_CTL, id, ctl0, ctl1);
}

_VALUES_IN_REGS ARG_STRUCT_T vm::drv_io (uint32_t id, drv_data_t *data)
{
    upc_(VMAPI_DRV_IO, id, (arch_word_t)data);
}

_VALUES_IN_REGS ARG_STRUCT_T vm::drv_probe (const char *name)
{
    upc_(VMAPI_DRV_PROBE, (arch_word_t)name);
}

_VALUES_IN_REGS ARG_STRUCT_T vm::lock (UINT32_T id)
{
    upc_(VMAPI_LOCK, id);
}    
_VALUES_IN_REGS ARG_STRUCT_T vm::unlock (UINT32_T id)
{
    upc_(VMAPI_UNLOCK, id);
}  

_VALUES_IN_REGS ARG_STRUCT_T vm::notify (const char *name)
{
    upc_(VMAPI_NOTIFY, (arch_word_t)name);
}

_VALUES_IN_REGS ARG_STRUCT_T vm::wait_notify ()
{
    upc_(VMAPI_WAIT_NOTIFY, 0);
}

_VALUES_IN_REGS ARG_STRUCT_T vm::notify_wait (const char *name)
{
    upc_(VMAPI_NOTIFY_WAIT, (arch_word_t)name);
}

_VALUES_IN_REGS ARG_STRUCT_T vm::sync (const char *name)
{
    upc_(VMAPI_SYNC, (arch_word_t)name);
}

_VALUES_IN_REGS ARG_STRUCT_T vm::wait (vm_thread_event_t cond)
{
    upc_(VMAPI_WAIT, (arch_word_t)cond);
}

_VALUES_IN_REGS ARG_STRUCT_T vm::wait_event (const char *event_name)
{
    upc_(VMAPI_WAIT_EVENT, (arch_word_t)event_name);
}

_VALUES_IN_REGS ARG_STRUCT_T vm::fire_event (const char *event_name)
{
    upc_(VMAPI_FIRE_EVENT, (arch_word_t)event_name);
}

_VALUES_IN_REGS ARG_STRUCT_T vm::mail (char *name, MAIL_HANDLE *mail)
{
    upc_(VMAPI_MAIL, (arch_word_t)name);
}

_VALUES_IN_REGS ARG_STRUCT_T vm::wait_mail ()
{
    upc_(VMAPI_WAIT_MAIL);
}

_VALUES_IN_REGS ARG_STRUCT_T vm::timer (arch_word_t *dest, arch_word_t id)
{
    upc_(VMAPI_TIMER_CREATE, arch_word_t(dest), id);
}

_VALUES_IN_REGS ARG_STRUCT_T vm::timer_remove (arch_word_t id)
{
    upc_(VMAPI_TIMER_REMOVE, id);
}

_VALUES_IN_REGS ARG_STRUCT_T vm::critical ()
{
    upc_(VMAPI_CRITICAL);
}

_VALUES_IN_REGS ARG_STRUCT_T vm::end_critical ()
{
    upc_(VMAPI_END_CRITICAL);
}

_VALUES_IN_REGS ARG_STRUCT_T vm::exit (UINT32_T ret)
{
    upc_(VMAPI_EXIT, ret);
}

_VALUES_IN_REGS ARG_STRUCT_T vm::fault (const char *cause)
{
    upc_(VMAPI_FAULT, (arch_word_t)cause);
}

_VALUES_IN_REGS ARG_STRUCT_T vm::reset ()
{
    upc_(VMAPI_RESET);
}


INT32_T VMAPI_ErrorHandler (arch_word_t _UNUSED(from), 
	_VALUES_IN_REGS ARG_STRUCT_T _UNUSED(arg))
{
    for (;;) {}
    //return 0;
}

/*End of file*/

