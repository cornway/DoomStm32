#ifndef __MACH_M4_H__
#define __MACH_M4_H__

#include <stdint.h>

#ifdef __cplusplus
    extern "C" {
#endif

#ifdef __ARMCC_VERSION
        

        
#else
#error "UNKNOWN COMPILER!"
#endif        
        
typedef uint64_t      arch_dword_t;
typedef uint32_t      arch_word_t;
typedef uint16_t      arch_hword_t;
typedef uint8_t       arch_byte_t; 

#define UINT32_T      uint32_t
#define INT32_T       int32_t
#define INT64_T       int64_t
#define UINT64_T      uint64_t


#define V_PREPACK  
#define V_POSTPACK __attribute__((packed))

#define _VALUES_IN_REGS     __value_in_regs

#define _WEAK __weak      
        
#define _STATIC static

#define _EXTERN extern

#define _UNUSED(a) a __attribute__((unused))

#define VOID

#define VM_FORCE_UPDATE 1

#define VM_CALL_FROM_USER   0
#define VM_CALL_FROM_IRQ    1

#define CPU_PRIV_ACCESS 0
#define CPU_UNPRIV_ACCESS 1

#define CPU_USE_MSP 0
#define CPU_USE_PSP 2

#define HANDLER_NOFPU_MSP   (0xF1U)
#define THREAD_NOFPU_MSP    (0xF9U)
#define THREAD_NOFPU_PSP    (0xFDU)
#define HANDLER_FPU_MSP     (0xE1U)
#define THREAD_FPU_MSP      (0xE9U)
#define THREAD_FPU_PSP      (0xEDU)

#define EXC_RETURN(exc)     (0xFFFFFF00U | exc)

#define EXC_RETURN_USE_FPU_BM   (0x10U)
#define EXC_RETURN_HANDLER_GM   (0xfU)
#define EXC_RETURN_HANDLER_VAL  (0x1)

#define FPU_STACK_SIZE      (33 * sizeof(arch_word_t))
#define CPU_STACK_SIZE      (17 * sizeof(arch_word_t))
    
#define STACK_ALLIGN        (8U)
    
#define CPU_XPSR_T_BM       (0x01000000U)

#define CPU_ACCESS_LEVEL_0 (CPU_USE_PSP | CPU_UNPRIV_ACCESS)
#define CPU_ACCESS_LEVEL_1 (CPU_USE_PSP | CPU_PRIV_ACCESS)
#define CPU_ACCESS_LEVEL_2 (CPU_USE_MSP | CPU_UNPRIV_ACCESS)
#define CPU_ACCESS_LEVEL_3 (CPU_USE_MSP | CPU_PRIV_ACCESS)


typedef INT32_T (*v_callback_t) (arch_word_t, void *);

typedef V_PREPACK struct {
    arch_word_t EXC_RET;
    arch_word_t R11; /*user top*/
    arch_word_t R10;
    arch_word_t R9;
    arch_word_t R8;
    arch_word_t R7;
    arch_word_t R6;
    arch_word_t R5;
    arch_word_t R4; /*irq top*/
    arch_word_t R0; 
    arch_word_t R1;
    arch_word_t R2;
    arch_word_t R3;
    arch_word_t R12;
    arch_word_t LR;
    arch_word_t PC;
    arch_word_t XPSR; /*pre irq top*/
    
} CPU_STACK; /*stack frame implementation for no fpu context store*/

typedef V_PREPACK struct {
    arch_word_t S16[16];
    arch_word_t EXC_RET;
    arch_word_t R11;
    arch_word_t R10;
    arch_word_t R9;
    arch_word_t R8;
    arch_word_t R7;
    arch_word_t R6;
    arch_word_t R5;
    arch_word_t R4;
    arch_word_t R0;
    arch_word_t R1;
    arch_word_t R2;
    arch_word_t R3;
    arch_word_t R12;
    arch_word_t LR;
    arch_word_t PC;
    arch_word_t XPSR;
    arch_word_t S[16];
    arch_word_t FPSCR;
} CPU_STACK_FPU; /*stack frame implementation for lazy fpu context store*/


typedef V_PREPACK struct {
    arch_word_t EXC_RET;
    arch_word_t RESERVED[8]; /*R10 - R4*/
    arch_word_t POINTER;     /*R0*/
    arch_word_t OPTION_A;    /*R1*/
    arch_word_t OPTION_B;    /*R2*/
    arch_word_t ERROR;       /*R3*/ 
    arch_word_t PAD;         /* */
    arch_word_t LINK;        /*Lr*/
    arch_word_t PC;          /*PC*/
    arch_word_t PSR;         /*XPSR*/
} CALL_CONTROL_CPU_STACK;

typedef V_PREPACK struct {
    arch_word_t RESERVED0[16];       /*S16 - S31*/
    arch_word_t EXC_RET;
    arch_word_t RESERVED[8];         /*R10 - R4*/
    arch_word_t POINTER;             /*R0*/
    arch_word_t OPTION_A;            /*R1*/
    arch_word_t OPTION_B;            /*R2*/
    arch_word_t ERROR;               /*R3*/ 
    arch_word_t PAD;                 /* */
    arch_word_t LINK;                /*Lr*/
    arch_word_t PC;                  /*PC*/
    arch_word_t PSR;                 /*XPSR*/
} CALL_CONTROL_CPU_FPU_STACK;


#pragma anon_unions

typedef V_PREPACK struct {
  V_PREPACK union {
        CPU_STACK      cpuStack;
        CPU_STACK_FPU  cpuStackFpu;
      
        CALL_CONTROL_CPU_STACK callControl;
        CALL_CONTROL_CPU_FPU_STACK callControlFpu;
    };
} CPU_STACK_FRAME;

typedef V_PREPACK struct {
   V_PREPACK union {
        V_PREPACK struct {
           arch_word_t R0;
           arch_word_t R1;
           arch_word_t R2;
           arch_word_t R3;
        };
        V_PREPACK struct {
           arch_word_t POINTER;
           arch_word_t CONTROL;
           arch_word_t LINK;
           arch_word_t ERROR;
        };
        V_PREPACK struct {
           arch_word_t PAD[3];
           arch_hword_t CTL;
           arch_hword_t IRQ;
        };
        V_PREPACK struct {
           CPU_STACK_FRAME *FRAME;
        };
    };
} ARG_STRUCT_T;


#define vm_ctxt_set_reg(frm, type, reg, val) \
        do { \
                        if (frm != NULL) { \
                            if ((type & EXC_RETURN_USE_FPU_BM) == 0) \
                                 frm->callControlFpu.reg = val; \
                            else \
                                 frm->callControl.reg = val; \
                        } \
         } while (0)
            
#define vm_ctxt_get_reg(frm, type, reg, var) \
        do { \
            if (frm != NULL) { \
                if ((type & EXC_RETURN_USE_FPU_BM) == 0) \
                        var = frm->callControlFpu.reg; \
                    else \
                        var = frm->callControl.reg; \
            } else var = 0; \
        } while (0)

#define THREAD_SET_REG(t, reg, val) \
    vm_ctxt_set_reg(t->CPU_FRAME, t->USE_FPU, reg, val)
                                
#define THREAD_GET_REG(t, reg, var) \
    vm_ctxt_get_reg(t->CPU_FRAME, t->USE_FPU, reg, var)

        
typedef struct {
    arch_word_t ACTLR;   /*Auxiliary Control Register                            */
    arch_word_t CPUID;   /*CPUID Base Register                                   */
    arch_word_t ICSR;    /*Interrupt Control and State Register                  */
    arch_word_t VTOR;    /*Vector Table Offset Register                          */
    arch_word_t AIRCR;   /*Application Interrupt and Reset Control Register      */
    arch_word_t SCR;     /*System Control Register                               */
    arch_word_t CCR;     /*Configuration and Control Register                    */
    arch_word_t SHPR1;   /*System Handler Priority Register 1                    */
    arch_word_t SHPR2;   /*System Handler Priority Register 2                    */
    arch_word_t SHPR3;   /*System Handler Priority Register 3                    */
    arch_word_t SHCRS;   /*System Handler Control and State Register             */
    arch_word_t CFSR;    /*Configurable Fault Status Register                    */
    arch_word_t MMSRb;   /*MemManage Fault Status Register                       */
    arch_word_t BFSRb;   /*BusFault Status Register                              */
    arch_word_t UFSRb;   /*UsageFault Status Register                            */
    arch_word_t HFSR;    /*HardFault Status Register                             */
    arch_word_t MMAR;    /*MemManage Fault Address Register                      */
    arch_word_t BFAR;    /*BusFault Address Register                             */
    arch_word_t AFSR;    /*Auxiliary Fault Status Register                       */
} SCB_M4_TypeDef;   /*system control block for cortex m4 core               */


void mach_m4_init_core_callback (void);

typedef void (*cpu_exc_handler_t) (void);

#pragma import import_mach_m4_systick_handler
#pragma import import_mach_m4_psv_handler
#pragma import import_mach_m4_svc_handler
#pragma import import_mach_m4_start_handler
#pragma import import_mach_m4_run_handler

_EXTERN void export_mach_m4_swrst (void);
_EXTERN _VALUES_IN_REGS ARG_STRUCT_T vm_init (void); /*from vm.cpp*/
_EXTERN _VALUES_IN_REGS ARG_STRUCT_T export_mach_m4_boot (void);
_EXTERN _VALUES_IN_REGS ARG_STRUCT_T export_mach_m4_svc (ARG_STRUCT_T);

#define arch_tick_alias                 export_mach_m4_sys_tick
#define arch_pend_alias                 export_mach_m4_psv
#define arch_hard_fault_alias           export_mach_m4_hard
#define arch_fpu_en_alias               export_mach_m4_fpu_en
#define arch_swrst_alias                export_mach_m4_swrst
#define arch_upcall_alias               export_mach_m4_svc
#define arch_boot_alias                 export_mach_m4_boot

#ifdef __cplusplus
    }
#endif



    
#endif /*__MACH_M4_H__*/


/*End of file*/

