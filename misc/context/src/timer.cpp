#include "timer.h"

_EXTERN "C" void *valloc (UINT32_T size);
_EXTERN "C" void vfree (void *p);

TIMER_HANDLE::TIMER_HANDLE ()
{
    this->dest = &this->count;
    this->count = 0;
}

TIMER_HANDLE::~TIMER_HANDLE ()
{
    
}


TIMER_FACTORY::TIMER_FACTORY ()
{
    this->mills = 0;
}

TIMER_FACTORY::~TIMER_FACTORY ()
{
    
}

void TIMER_FACTORY::tick_ms ()
{
    this->mills++;
    if ((mills & 999) == 0) {
        this->tick_s();
    }
    TIMER_HANDLE *timer = this->timers_ms.getFirst();
    while (timer != nullptr) {
        timer->count++;
        *(timer->dest) = timer->count;
        timer = timer->next();
    }
}

void TIMER_FACTORY::tick_s ()
{
    TIMER_HANDLE *timer = this->timers_s.getFirst();
    while (timer != nullptr) {
        timer->count++;
        *(timer->dest) = timer->count;
        timer = timer->next();
    }
}

INT32_T TIMER_FACTORY::create (arch_word_t *dest, arch_word_t id)
{
    if (dest == nullptr) {
        return -1;
    }
    TIMER_HANDLE *timer = (TIMER_HANDLE *)valloc(sizeof(TIMER_HANDLE));
    if (timer == nullptr) {
        return -1;
    }
    timer->dest = dest;
    timer->count = 0;
    if ((id & TIMER_SECONDS_MASK) == TIMER_SECONDS_MASK) {
        this->timers_s.addFirst(timer);
    } else {
        this->timers_ms.addFirst(timer);
    }
    return 0;
}

INT32_T TIMER_FACTORY::remove (arch_word_t id)
{
    vector::Vector<TIMER_HANDLE> *list;
    if ((id & TIMER_SECONDS_MASK) == TIMER_SECONDS_MASK) {
        list = &this->timers_s;
    } else {
        list = &this->timers_ms;
    }
    TIMER_HANDLE *timer = list->getFirst();
    while (timer != nullptr) {
        if (timer->id == id) {
            list->remove(timer);
            vfree(timer);
            return 0;
        }
        timer = timer->next();
    }
    return -1;
}

