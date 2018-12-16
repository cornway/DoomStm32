#ifndef VMAPI_CALL_H
#define VMAPI_CALL_H

#ifdef __cplusplus
    extern "C" {
#endif
        
#define DEPREC_IDLE_CALL    (0x80U)
        
enum {
    VMAPI_RESTART       = 0,
    VMAPI_SLEEP         = 1 | DEPREC_IDLE_CALL,
    VMAPI_YIELD         = 2,
    VMAPI_CREATE        = 3,
    VMAPI_LOCK          = 4 | DEPREC_IDLE_CALL,
    VMAPI_UNLOCK        = 5 | DEPREC_IDLE_CALL,
    VMAPI_NOTIFY        = 6,
    VMAPI_WAIT_NOTIFY   = 7 | DEPREC_IDLE_CALL,
    VMAPI_NOTIFY_WAIT   = 8 | DEPREC_IDLE_CALL,
    VMAPI_SYNC          = 9 | DEPREC_IDLE_CALL,
    VMAPI_WAIT          = 10 | DEPREC_IDLE_CALL,
    VMAPI_WAIT_EVENT    = 11 | DEPREC_IDLE_CALL,
    VMAPI_FIRE_EVENT    = 12,
    VMAPI_MAIL          = 13,
    VMAPI_WAIT_MAIL     = 14 | DEPREC_IDLE_CALL,
    VMAPI_TIMER_CREATE  = 15,
    VMAPI_TIMER_REMOVE  = 16,
    VMAPI_CALL          = 17,
    VMAPI_CRITICAL      = 18,
    VMAPI_END_CRITICAL  = 19,
    VMAPI_DRV_ATTACH    = 20,
    VMAPI_DRV_DETTACH   = 21,
    VMAPI_DRV_IO        = 22,
    VMAPI_DRV_CTL       = 23,
    VMAPI_DRV_PROBE     = 24,

    VMAPI_FAULT         = 25,
    VMAPI_EXIT          = 26,

    VMAPI_RESET         = 0x80000U,
};    
        
        
#ifdef __cplusplus
    }
#endif
        
#endif /*VMAPI_CALL_H*/
