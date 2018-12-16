#ifndef VM_TIMER_H
#define VM_TIMER_H

#include "arch.h"
#include "vm_conf.h"
#include "iterable.h"

#define TIMER_SECONDS_MASK  (0x80000000U) /*need to identify what timer is create - milisecond or second accuracy*/

class TIMER_FACTORY;

class TIMER_HANDLE : public Link<TIMER_HANDLE> {
    private :
        arch_word_t *dest;
        arch_word_t count;
        arch_word_t id;
    
        TIMER_HANDLE ();
        ~TIMER_HANDLE ();
        friend class TIMER_FACTORY;
    public :
        
};

class TIMER_FACTORY {
    private :
        vector::Vector<TIMER_HANDLE> timers_ms, timers_s;
        arch_word_t mills;
    
        void tick_s ();
    public :
        TIMER_FACTORY ();
        ~TIMER_FACTORY ();
    
        void tick_ms ();    
        INT32_T create (arch_word_t *dest, arch_word_t id);
        INT32_T remove (arch_word_t id);
};

#endif /*VM_TIMER_H*/
