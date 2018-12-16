                    PRESERVE8 
                    EXPORT export_mach_m4_sys_tick
                    EXPORT export_mach_m4_psv
                    EXPORT export_mach_m4_hard
                    EXPORT export_mach_m4_fpu_en
                    EXPORT export_mach_m4_swrst
                    EXPORT export_mach_m4_svc                  [WEAK]
                    EXPORT export_mach_m4_svc_hdlr
                    EXPORT export_mach_m4_boot

                        
                    AREA    |.text|, CODE, READONLY   
                    IMPORT import_mach_m4_systick_handler
                    IMPORT import_mach_m4_svc_handler
                    IMPORT import_mach_m4_start_handler
                    IMPORT import_mach_m4_run_handler
                    IMPORT import_mach_m4_hard_handler
                    IMPORT Reset_Handler

                    MACRO 
$label              WRAP $DEST
                
                    CPSID   I
                    DMB
                    LDR     R3, =0
                    TST     LR, #0x04
                    BNE     $label.STORE_PSP
                    MRS     R0, MSP 
                    B       $label.BRS
$label.STORE_PSP    MRS     R0, PSP                    
$label.BRS          MOV     R2, LR
                    MRS     R1, CONTROL
                    STMDB   R0!, {R4 - R11, LR}
                    TST     LR, #0x10 ;check fpu
                    BNE     $label.FPU_SKIP
                    VSTMDB  R0!, {S16 - S31}
$label.FPU_SKIP     BL      $DEST
                    TST     R2, #0x10
                    BNE     $label.FPU_SKIP_
                    VLDMIA  R0!, {S16 - S31}
$label.FPU_SKIP_    LDMIA   R0!, {R4 - R11, LR} 
                    TST     R1, #0x02
                    BNE     $label.LOAD_PSP
                    MSR     MSP, R0
                    B       $label.BRL
$label.LOAD_PSP     MSR     PSP, R0 
                    DMB
$label.BRL          CPSIE   I
                    MSR     CONTROL, R1
                    BX      R2
                
                    MEND

export_mach_m4_svc  FUNCTION
                    SWI     0x02
                    BX      LR
                    ENDP
                        
                        
export_mach_m4_boot FUNCTION  
                    CPSID   I
                    DSB
                    BL      export_mach_m4_fpu_en
                    BL      import_mach_m4_run_handler  
                    DMB
                    MSR     PSP, R0
                    CPSIE   I
                    MSR     CONTROL, R1
                    POP     {PC}
                    ENDP
                        
export_mach_m4_sys_tick FUNCTION 
SYS_TICK_           WRAP import_mach_m4_systick_handler                
                    ENDP  
                        
export_mach_m4_psv  FUNCTION  
                    B .
                    ENDP  
                        
export_mach_m4_svc_hdlr  FUNCTION   
SVC_HANDLE_         WRAP import_mach_m4_svc_handler
                    ENDP 
                    
export_mach_m4_hard    FUNCTION  
HARD_FAULT_         WRAP import_mach_m4_hard_handler   
                    ENDP 
TableEnd        
                
export_mach_m4_fpu_en  FUNCTION
                    ; CPACR is located at address 0xE000ED88
                    LDR R0, =0xE000ED88
                    ; Read CPACR
                    LDR R1, [R0]
                    ; Set bits 20-23 to enable CP10 and CP11 coprocessors
                    ORR R1, R1, #(0xF << 20)
                    ; Write back the modified value to the CPACR
                    STR R1, [R0]; wait for store to complete
                    DSB
                    ;reset pipeline now the FPU is enabled
                    ISB
                    
                    LDR R0, =0xE000EF34 ;FPCCR
                    LDR R1, [R0]
                    ORR R1, R1, #(0x3 << 30) ;set bits 30-31 to enable lazy staking and automatic status store
                    STR R1, [R0]
                    DSB
                    
                    BX  LR
                    ENDP  
                    
export_mach_m4_swrst   FUNCTION
                    DSB
                    LDR R0, =0xE000ED0C ;AIRCR
                    LDR R1, =0x05FA0004
                    STR R1, [R0]
                    DSB
                    B .
                    ENDP
                    END
                       
                
                
                
                
                
                
                
                
                
                
                
                
                
                