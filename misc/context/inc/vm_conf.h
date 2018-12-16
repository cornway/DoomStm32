#ifndef VM_CONF_H
#define VM_CONF_H

#include "arch.h"

#define     VM_THREAD_MAX_COUNT (24U)

#define     VM_THREAD_MAX_PRIORITY  (8U)

#define     VM_THREAD_DEF_PRIORITY  (3U)

#define     VM_DEF_THREAD_HEAP_SIZE (6144U)

#define     VM_IDLE_THREAD_HEAP_SIZE (8192U)

#define     VM_DEF_THREAD_NAME_LEN (24U)

#define     VM_IDLE_THREAD_ID  (0U)

#define VM_MAX_OWNERS_COUNT    (12U)

#define VM_MAX_MUTEX_COUNT     (12U)

#define VM_MAX_DRIVERS (10)

#define VM_PREEMT_SWITCH_TIME_MS (20)

#define VM_PROFILE_TIME_MS (2000)


#endif /*VM_CONF_H*/



