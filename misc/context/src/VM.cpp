#include "VM.h"
#include "thread.h"
#include "mail.h"
#include "timer.h"
#include "driver.h"
#include "mutex.h"
#include "vmapi_call.h"
#include "string.h"

_EXTERN "C" void *valloc (UINT32_T size);
_EXTERN "C" void vfree (void *p);
_STATIC _VALUES_IN_REGS ARG_STRUCT_T vm_dispatch_tick (ARG_STRUCT_T ARG);
_STATIC _VALUES_IN_REGS ARG_STRUCT_T vm_dispatch_svc (ARG_STRUCT_T arg);
_STATIC _VALUES_IN_REGS ARG_STRUCT_T vm_vm ();
_STATIC arch_word_t vm_set_link (struct vm_thread_desc *t);
_STATIC struct vm_thread_desc *vm_get_next ();
_STATIC void vm_set_stack_var (struct vm_thread_desc *t, ARG_STRUCT_T arg);
_STATIC struct vm_thread_desc *vm_pick_ready ();
_STATIC _VALUES_IN_REGS ARG_STRUCT_T vm_update_vars (struct vm_thread_desc *t);


_STATIC struct vm_thread_desc *g_vm_current_thread = NULL;
_STATIC struct vm_thread_desc *g_vm_idle_thread = NULL;

_STATIC UINT64_T g_vm_uWTick = 0U;
_STATIC UINT64_T g_vm_uWtickPrev = 0U;
_STATIC bool g_vm_preemtSwitchEnabled = true;

_STATIC MUTEX_FACTORY g_vm_mutexFactory;
_STATIC TIMER_FACTORY g_vm_timerFactory;

_VALUES_IN_REGS ARG_STRUCT_T vm_init ()
{
    ARG_STRUCT_T ret = {0, 0, 0, 0};
    static struct vm_thread_desc *t = NULL;
    
    mach_m4_init_core_callback();
    t_init_core();
    
    int res = t_init (          &t, 
                                VM_SYS_THREAD,
                                VM_THREAD_MAX_PRIORITY - 1,
                                IDLE_THREAD_ID,
                                VM_IDLE_THREAD_HEAP_SIZE,
                                CPU_ACCESS_LEVEL_1,
                                "SYS_IDLE",
                                0,
                                NULL,
                                1
                            );
    V_ASSERT(res == T_OK);
    t_link_ready(t);
    g_vm_idle_thread = t;
    t->USE_FPU = THREAD_NO_FPU;
    ret.R0 = VM_OK;
    HAL_InitTick(TICK_INT_PRIORITY);
    return ret;
}

_VALUES_IN_REGS ARG_STRUCT_T VMBREAK (UINT32_T ret)
{
	V_ASSERT(g_vm_current_thread->ID != IDLE_THREAD_ID);
    ARG_STRUCT_T arg = {VMAPI_EXIT, ret, ret, 0};
    return arch_upcall_alias(arg);
}

_STATIC arch_word_t vm_set_link (struct vm_thread_desc *t)
{
    if (t->USE_FPU == THREAD_FPU) {
        if ((t->PRIVILEGE & CPU_USE_PSP) == CPU_USE_PSP) {
            return EXC_RETURN(THREAD_FPU_PSP);
        } else {
            return EXC_RETURN(THREAD_FPU_MSP);
        }
    } else {
        if ((t->PRIVILEGE & CPU_USE_PSP) == CPU_USE_PSP) {
            return EXC_RETURN(THREAD_NOFPU_PSP);
        } else {
            return EXC_RETURN(THREAD_NOFPU_MSP);
        }
    }
}

_STATIC void vm_set_stack_var (struct vm_thread_desc *t, ARG_STRUCT_T arg)
{
    if ((arg.LINK & EXC_RETURN_USE_FPU_BM) == 0) { /**/
        t->USE_FPU = THREAD_FPU;
    } else {
        t->USE_FPU = THREAD_NO_FPU;
    }
    t->CPU_FRAME = arg.FRAME;
}

_STATIC _VALUES_IN_REGS ARG_STRUCT_T vm_update_vars (struct vm_thread_desc *t)
{
    ARG_STRUCT_T ret = {0, 0, 0, 0};
    ret.LINK = vm_set_link(t); /*set link register value*/
    ret.CONTROL = t->PRIVILEGE;
    ret.FRAME = t->CPU_FRAME;
    return ret;
}

_STATIC struct vm_thread_desc *vm_get_next ()
{
    struct vm_thread_desc *t = NULL;
    for (UINT32_T i = 0; i < VM_THREAD_MAX_PRIORITY; i++) {
        t = t_ready(i);
        if (t != NULL) {
            break;
        }
    }
    return t;
}
_STATIC struct vm_thread_desc *vm_pick_ready ()
{
    struct vm_thread_desc *t;
    
    if (t_check_list() == 0) {
        t_refresh( t_link_ready );
    }
    t = vm_get_next();
	V_ASSERT(t != NULL);
    if (t->STATUS != THREAD_STOP) {
        t_unchain(t_link_ready, t);
    } 
    t->STATUS = THREAD_RUN;
    return t;
}

_STATIC _VALUES_IN_REGS ARG_STRUCT_T vm_vm ()
{
    ARG_STRUCT_T ret = {0, 0, 0, 0};
    
    g_vm_current_thread = g_vm_idle_thread;
    
    ret.CONTROL = g_vm_idle_thread->PRIVILEGE;
    ret.FRAME = g_vm_idle_thread->CPU_FRAME;
    return ret;
}

#if 0
_STATIC INT32_T vm_profile_do_tick ()
{
    static UINT32_T last_profile_time = 0;
    UINT32_T profile_dur = g_vm_uWTick - last_profile_time;
    if (profile_dur < VM_PROFILE_TIME_MS) {
        return 1;
    }
    last_profile_time = g_vm_uWTick;
    UINT32_T  wage;
    struct vm_thread_desc *t = NULL;
    for (INT32_T i = 0; i < THREAD_MAX_COUNT; i++) {
        t = g_vm_thread_table[i];
        if (t && (t != g_vm_idle_thread)) {
            wage = ((profile_dur << 3) / g_vm_thread_table[i]->cpuUsage ) >> 3;
            switch (
                wage <= (VM_THREAD_MAX_PRIORITY / t->PRIORITY) + 1 ?
                    (wage >= (VM_THREAD_MAX_PRIORITY / t->PRIORITY) - 1 ?
                        (wage >= (VM_THREAD_MAX_PRIORITY / t->PRIORITY) ? 2 :  1 ) :
                        0
                    ) : 0
            ) {
                case 1:
                    if (t->PRIORITY)
                        t->PRIORITY--;
                    break;
                case 2: 
                    if (t->PRIORITY < VM_THREAD_MAX_PRIORITY - 1)
                        t->PRIORITY++;
                    break;
            }
        }
    }
    last_profile_time = g_vm_uWTick;
    return 0;
}
#endif

_STATIC _VALUES_IN_REGS ARG_STRUCT_T vm_dispatch_tick (ARG_STRUCT_T arg)
{
    g_vm_uWTick++;
    struct vm_thread_desc *t = g_vm_current_thread;
    ARG_STRUCT_T ret = {0, 0, 0, 0};
    
    g_vm_timerFactory.tick_ms();
    
    if (((arg.LINK & EXC_RETURN_HANDLER_GM) == EXC_RETURN_HANDLER_VAL) || ((g_vm_uWTick % VM_PREEMT_SWITCH_TIME_MS) == 0) || (g_vm_preemtSwitchEnabled == 0)) { /*return, if another IRQ is pending*/
        ret.POINTER = arg.POINTER;
        ret.LINK    = arg.LINK;
        ret.CONTROL = t->PRIVILEGE;
        return ret;
    }
    
    vm_set_stack_var(t, arg);
    t_unlink_ready(t);
    t->cpuUsage += g_vm_uWTick - g_vm_uWtickPrev;
    g_vm_uWtickPrev = g_vm_uWTick;
    t->STATUS = THREAD_PEND;
    t->V_PRIORITY++;
    if (t->V_PRIORITY < VM_THREAD_MAX_PRIORITY) {
        t_link_ready(t);
    } else {
        t_link_drop(t);
    } 
    
    t_check_cond( t_link_ready );
    t_tick( t_link_drop );
    /*vm_profile_do_tick();*/
    g_vm_current_thread = vm_pick_ready();
    
    return vm_update_vars(g_vm_current_thread);
}

#define DISPATCH_RET(IRQ, ret) \
if (IRQ == VM_CALL_FROM_IRQ) { \
    return ret; \
}

#define DISPATCH_IS_IRQ(IRQ) \
(IRQ == VM_CALL_FROM_IRQ)

_VALUES_IN_REGS ARG_STRUCT_T vm_dispatch_svc (ARG_STRUCT_T arg)
{

    ARG_STRUCT_T call_struct = {0, 0, 0, 0};
    CPU_STACK_FRAME *frame = NULL;

    ARG_STRUCT_T ret;
    ret.POINTER = arg.POINTER;
    ret.LINK    = arg.LINK;
    ret.ERROR   = arg.ERROR;
    ret.CONTROL = g_vm_current_thread->PRIVILEGE;

    if (arg.IRQ == VM_CALL_FROM_USER) {
        vm_set_stack_var(g_vm_current_thread, arg);
        frame = g_vm_current_thread->CPU_FRAME;
        /*
	       call_struct.R0[7 : 0] - call reason, if == 0 -> thread restart;
	       call_struct.R1 - pointer to handle struct;
	       call_struct.R2 - attribute 0;
	       call_struct.R2 - attribute 1;
	       */
        vm_ctxt_get_reg(frame, arg.LINK, POINTER, call_struct.R0);
        vm_ctxt_get_reg(frame, arg.LINK, OPTION_A, call_struct.R1);
        vm_ctxt_get_reg(frame, arg.LINK, OPTION_B, call_struct.R2);
        vm_ctxt_get_reg(frame, arg.LINK, ERROR, call_struct.R3);
        /*
        	frame->cpuStack.R0 - pointer to resource
        	frame->cpuStack.R1 - attribute 1
        	frame->cpuStack.R2 - attribute 2
        	frame->cpuStack.R3 - error code
        	*/

    } else if (arg.IRQ == VM_CALL_FROM_IRQ) {
        call_struct.R0 = arg.R0;
        call_struct.R1 = arg.R1;
        call_struct.R2 = arg.R2;
        call_struct.R3 = arg.R3;
    }

    if (call_struct.R0 == VMAPI_RESET) {
        arch_swrst_alias();
    } else {
        arch_byte_t reason = call_struct.R0 & 0xFFU;
        if ((reason & DEPREC_IDLE_CALL) == DEPREC_IDLE_CALL) {
            if  (g_vm_current_thread->ID == IDLE_THREAD_ID) {
                DISPATCH_RET(arg.IRQ, ret);
                vm_ctxt_set_reg(frame, arg.LINK, ERROR, VM_DEPRECATED_CALL);
                return ret;
            }
        }
        reason = call_struct.R0;
        
        arch_byte_t force_ctx_switch = false;
        int32_t res = VM_OK;
        drv_t driver_handler;
        static arch_byte_t reentrance = 0;
		arch_word_t mutex_res;
		struct vm_thread_desc *thread_pvar_desc;
        vthread_t *thread_pvar;
        switch (reason) {
            case VMAPI_SLEEP :  g_vm_current_thread->tdesc_wait_cnt = call_struct.R1;
                                t_unlink_ready(g_vm_current_thread);
                                t_link_delay(g_vm_current_thread);
                                force_ctx_switch = true;
                break;
            case VMAPI_YIELD :  t_unlink_ready(g_vm_current_thread);
                                t_link_drop(g_vm_current_thread);
                                force_ctx_switch = true;
                break;
            case VMAPI_CREATE : thread_pvar = (vthread_t *)call_struct.R1;
                                res = t_init (
                                                        &thread_pvar_desc, 
                                                        (v_callback_t)thread_pvar->t_entry_point,
                                                        thread_pvar->Priority,
                                                        (arch_word_t)thread_pvar->t_entry_point,
                                                        thread_pvar->StackSize,
                                                        CPU_ACCESS_LEVEL_0,
                                                        thread_pvar->Name,
                                                        thread_pvar->argSize,
                                                        thread_pvar->Arg,
                                                        0
                                                        );
                                if (res != VM_OK) {
                                    vm_ctxt_set_reg(frame, arg.LINK, ERROR, res);
                                    break;
                                }
                                t_link_ready(thread_pvar_desc);
                                vm_ctxt_set_reg(frame, arg.LINK, ERROR, VM_OK);
                                break;
                //break;
            case VMAPI_CALL :   thread_pvar = (vthread_t *)call_struct.R1;
                                res = t_init (
                                                        &thread_pvar_desc, 
                                                        (v_callback_t)thread_pvar->t_entry_point,
                                                        thread_pvar->Priority,
                                                        (arch_word_t)thread_pvar->t_entry_point,
                                                        thread_pvar->StackSize,
                                                        CPU_ACCESS_LEVEL_0,
                                                        thread_pvar->Name,
                                                        thread_pvar->argSize,
                                                        thread_pvar->Arg,
                                                        0
                                                        );
                                if (res != VM_OK) {
                                    vm_ctxt_set_reg(frame, arg.LINK, ERROR, res);
                                    break;
                                }
                                t_link_ready(thread_pvar_desc);
                                t_unlink_ready(g_vm_current_thread);
                                thread_pvar_desc->caller = g_vm_current_thread;
                                force_ctx_switch = true;
                                break;
            case VMAPI_LOCK :   mutex_res = g_vm_mutexFactory.lock(g_vm_current_thread, call_struct.R1);
                                if (mutex_res == MUTEX_GRANT_LOCK) { /*succeed locked*/
                                    vm_ctxt_set_reg(frame, arg.LINK, ERROR, VM_OK);
                                } else if (mutex_res == MUTEX_GRANT_WAIT) { /*thread will be unlinked and queued*/
                                    t_unlink_ready(g_vm_current_thread);
                                    g_vm_current_thread->mutex = true;
                                    force_ctx_switch = true;
                                } else {
                                    t_unlink_ready(g_vm_current_thread);
                                    t_link_fault(g_vm_current_thread);
                                    g_vm_current_thread->fault = true;
                                    g_vm_current_thread->faultMessage = "Cannot allocate mutex resource";
                                    force_ctx_switch = true;
                                }
                                
                break;
            case VMAPI_UNLOCK : thread_pvar_desc = g_vm_mutexFactory.unlock(g_vm_current_thread, call_struct.R1);
                                if (thread_pvar_desc != nullptr) { /*set next owner and unlock mutex for it*/
                                    t_link_ready(thread_pvar_desc);
                                    thread_pvar_desc->mutex = false;
                                    g_vm_current_thread = thread_pvar_desc;
                                    ret = vm_update_vars(g_vm_current_thread);
                                    break;
                                }
                                vm_ctxt_set_reg(frame, arg.LINK, ERROR, VM_OK);
                                break;
                                
            case VMAPI_NOTIFY :   thread_pvar_desc = t_notify((const char *)call_struct.R1);
                                  if ((thread_pvar_desc != nullptr) && (thread_pvar_desc->waitNotify == 1)) {
                                      thread_pvar_desc->waitNotify = 0;
                                      t_unlink_notify(thread_pvar_desc);
                                      t_link_ready(thread_pvar_desc);
                                      vm_ctxt_set_reg(frame, arg.LINK, ERROR, VM_OK);
                                  } else {
                                    vm_ctxt_set_reg(frame, arg.LINK, ERROR, VM_NOT_FOUND);
                                  }
                                  break;
                //break;
            case VMAPI_SYNC :   thread_pvar_desc = t_search((const char *)call_struct.R1);
                                if (thread_pvar_desc == nullptr) {
                                    break;
                                }
                                if (thread_pvar_desc->fault) {
                                    break;
                                }
                                t_unlink_ready(g_vm_current_thread);
                                t_link_chain(thread_pvar_desc, g_vm_current_thread);
                                force_ctx_switch = true;
                                break;
                //break;
            case VMAPI_NOTIFY_WAIT :  thread_pvar_desc = t_notify((const char *)call_struct.R1);
                                      if ((thread_pvar_desc != NULL) && (thread_pvar_desc->waitNotify == true)) {
                                          thread_pvar_desc->waitNotify = false;
                                          t_unlink_notify(thread_pvar_desc);
                                          t_link_ready(thread_pvar_desc);
                                      }
                                      if (g_vm_current_thread->waitNotify == false) {
                                          g_vm_current_thread->waitNotify = true;
                                          t_unlink_ready(g_vm_current_thread);
                                          t_link_notify(g_vm_current_thread);
                                      } else {
                                          g_vm_current_thread->fault = true;
                                          t_unlink_ready(g_vm_current_thread);
                                          t_link_fault(g_vm_current_thread);
                                          vm_ctxt_set_reg(frame, arg.LINK, ERROR, VM_DUP_CALL);
                                      }
                                      vm_ctxt_set_reg(frame, arg.LINK, ERROR, VM_OK);
                break;
            case VMAPI_WAIT_NOTIFY :    if (g_vm_current_thread->waitNotify == true) { /*error*/
                                            g_vm_current_thread->fault = true;
                                            t_unlink_ready(g_vm_current_thread);
                                            t_link_fault(g_vm_current_thread);
                                            vm_ctxt_set_reg(frame, arg.LINK, ERROR, VM_DUP_CALL);
                                            break;
                                        } else {
                                            g_vm_current_thread->waitNotify = 1;
                                            t_unlink_ready(g_vm_current_thread);
                                            t_link_notify(g_vm_current_thread);
                                            vm_ctxt_set_reg(frame, arg.LINK, ERROR, VM_OK);
                                        }
                                        force_ctx_switch = true;
                break;
            case VMAPI_WAIT : g_vm_current_thread->cond = (vm_thread_event_t)call_struct.R1;
                              t_unlink_ready(g_vm_current_thread);
                              t_link_cond(g_vm_current_thread);
                              force_ctx_switch = true;
                break;
            case VMAPI_WAIT_EVENT : g_vm_current_thread->event_name = (const char *)call_struct.R1;
                                    t_unlink_ready(g_vm_current_thread);
                                    t_link_wait_event(g_vm_current_thread);
                                    vm_ctxt_set_reg(frame, arg.LINK, ERROR, VM_OK);
                                    force_ctx_switch = true;
                                    break;
                //break;
            case VMAPI_FIRE_EVENT : t_fire_event(t_link_ready, (const char *)call_struct.R1);
                                    vm_ctxt_set_reg(frame, arg.LINK, ERROR, VM_OK);
                                    break;
                //break;
            case VMAPI_WAIT_MAIL :      if (g_vm_current_thread->waitNotify == true) { /*error*/
                                            g_vm_current_thread->fault = true;
                                            t_unlink_ready(g_vm_current_thread);
                                            t_link_fault(g_vm_current_thread);
                                            break;
                                        } else {
                                            g_vm_current_thread->waitNotify = 1;
                                            t_unlink_ready(g_vm_current_thread);
                                            t_link_mail(g_vm_current_thread);
                                        }
                                        force_ctx_switch = true;
                break;
            case VMAPI_MAIL :     thread_pvar_desc = t_notify((const char *)call_struct.R1);
                                  if ((thread_pvar_desc != nullptr) && (thread_pvar_desc->waitNotify == true)) {
                                      thread_pvar_desc->waitNotify = false;
                                      t_unlink_mail(thread_pvar_desc);
                                      t_link_ready(thread_pvar_desc);
                                      THREAD_SET_REG(thread_pvar_desc, POINTER, call_struct.R2);
                                  } else {
                                    vm_ctxt_set_reg(frame, arg.LINK, ERROR, VM_NOT_FOUND);
                                  }
                                  break;
                //break;
            case VMAPI_TIMER_CREATE :   if (g_vm_timerFactory.create((arch_word_t *)call_struct.R1, call_struct.R2) < 0) {
                                            vm_ctxt_set_reg(frame, arg.LINK, ERROR, VM_CREATE_ERR);
                                        } else {
                                            vm_ctxt_set_reg(frame, arg.LINK, ERROR, VM_OK);
                                        }
                                        
                break;
            case VMAPI_TIMER_REMOVE :   if (g_vm_timerFactory.remove(call_struct.R1) < 0) {
                                            vm_ctxt_set_reg(frame, arg.LINK, ERROR, VM_NOT_FOUND);
                                        } else {
                                            vm_ctxt_set_reg(frame, arg.LINK, ERROR, VM_OK);
                                        }
                break;
            case VMAPI_CRITICAL : g_vm_preemtSwitchEnabled = false;
                break;
            case VMAPI_END_CRITICAL : g_vm_preemtSwitchEnabled = true;
                break;                      
            case VMAPI_DRV_ATTACH : res = drv_attach((drv_handle_t *)call_struct.R1, call_struct.R2, call_struct.R3);
                                    if (res < 0) {
                                        vm_ctxt_set_reg(frame, arg.LINK, ERROR, VM_CREATE_ERR);
                                    } else {
                                        vm_ctxt_set_reg(frame, arg.LINK, ERROR, res);
                                    }
                break;
            case VMAPI_DRV_DETTACH :res = drv_detach(call_struct.R1);
                                    if (res < 0) {
                                        vm_ctxt_set_reg(frame, arg.LINK, ERROR, VM_CREATE_ERR);
                                    } else {
                                        vm_ctxt_set_reg(frame, arg.LINK, ERROR, res);
                                    }
                break;              
            case VMAPI_DRV_IO :     driver_handler = drv_get(call_struct.R1);
                                    if (driver_handler.id == 0) {
                                        vm_ctxt_set_reg(frame, arg.LINK, ERROR, VM_CREATE_ERR);
                                        break;
                                    } 
                                    if (driver_handler.handle->io != NULL) {
                                        res = driver_handler.handle->io(NULL, call_struct.R2, (drv_data_t *)call_struct.R3);
                                    }
                                    vm_ctxt_set_reg(frame, arg.LINK, ERROR, res);
                break;
            case VMAPI_DRV_CTL :    driver_handler = drv_get(call_struct.R1);
                                    if (driver_handler.id == 0) {
                                        vm_ctxt_set_reg(frame, arg.LINK, ERROR, VM_CREATE_ERR);
                                        break;
                                    } 
                                    if (driver_handler.handle->ioctl != NULL) {
                                        res = driver_handler.handle->ioctl((void *)call_struct.R3, (void *)call_struct.R2, NULL);
                                    }
                                    vm_ctxt_set_reg(frame, arg.LINK, ERROR, res);
                break;   
            case VMAPI_DRV_PROBE:   res = drv_get_id((const char *)call_struct.R1);
                                    vm_ctxt_set_reg(frame, arg.LINK, ERROR, res);
                break;
            case VMAPI_FAULT :  g_vm_current_thread->fault = true;
                                t_unlink_ready(g_vm_current_thread);
                                force_ctx_switch = true;
                break;
            case VMAPI_EXIT :   t_unlink_ready(g_vm_current_thread);
                                thread_pvar_desc = VM_THREAD_DESC(g_vm_current_thread->caller);
                                if (thread_pvar_desc != NULL) {
                                    THREAD_SET_REG(thread_pvar_desc, OPTION_B, call_struct.R1);
                                    t_link_ready(thread_pvar_desc);
                                }
                                t_destroy(g_vm_current_thread);
                                g_vm_current_thread = thread_pvar_desc;
                                force_ctx_switch = true;
                break;
            default :   /*TODO: Add case*/
                        vm_ctxt_set_reg(frame, arg.LINK, ERROR, VM_UNKNOWN_CALL);
                        break;
                //break;
        }

        if ((force_ctx_switch || reentrance) && !DISPATCH_IS_IRQ(arg.IRQ)) {
            g_vm_current_thread = vm_pick_ready();
            ret.LINK = vm_set_link(g_vm_current_thread);
            ret.FRAME = g_vm_current_thread->CPU_FRAME;
            ret.CONTROL = g_vm_current_thread->PRIVILEGE;
            reentrance = false;
            return ret;
        }
        reentrance = DISPATCH_IS_IRQ(arg.IRQ) ? force_ctx_switch : false;
        
        return ret;
    }
    return ret;
}

extern "C"
_VALUES_IN_REGS ARG_STRUCT_T import_mach_m4_systick_handler (ARG_STRUCT_T arg)
{
    return vm_dispatch_tick(arg);
}

extern "C"
_VALUES_IN_REGS ARG_STRUCT_T import_mach_m4_start_handler (ARG_STRUCT_T arg)
{
    ARG_STRUCT_T ret = {arg.R0, arg.R1, arg.R2, arg.R3};
    return ret;
}

extern "C"
_VALUES_IN_REGS ARG_STRUCT_T import_mach_m4_run_handler ()
{
    vm_init();
    return vm_vm();
}

extern "C"
_VALUES_IN_REGS ARG_STRUCT_T import_mach_m4_svc_handler (ARG_STRUCT_T arg)
{
    return vm_dispatch_svc(arg);
}

extern "C"
_VALUES_IN_REGS ARG_STRUCT_T import_mach_m4_hard_handler (ARG_STRUCT_T arg)
{
    for (;;) {
        
    }
}

extern "C"
void *import_mach_m4_psv_handler (void *frame, int32_t link)
{
    return frame;
}


