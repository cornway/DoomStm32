#include "mutex.h"



OWNER::OWNER ()
{
    this->busy = false;
}

OWNER::~OWNER ()
{
    
}

OWNER::OWNER (struct vm_thread_desc *owner)
{
    this->owner = owner;
}

OWNER::OWNER (OWNER &owner)
{
    this->owner = owner.owner;
}

void OWNER::init (struct vm_thread_desc *t)
{
    this->owner = t;
}

void OWNER::operator = (OWNER &owner)
{
    this->owner = owner.owner;
}

void OWNER::operator = (OWNER *owner)
{
    this->owner = owner->owner;
}

struct vm_thread_desc *OWNER::getOwner ()
{
    return this->owner;
}


MUTEX::MUTEX ()
{
    this->busy = false;
}


MUTEX::~MUTEX ()
{
    
}


void MUTEX::init (arch_word_t id)
{
    this->Id = id;
}




arch_word_t MUTEX::lock (OWNER *owner)
{
    this->owners.addLast(owner);
    return MUTEX_GRANT_WAIT;
}

arch_word_t MUTEX::lock (struct vm_thread_desc *t)
{
    this->owner = t;
    return MUTEX_GRANT_LOCK;
}


OWNER *MUTEX::unlock (struct vm_thread_desc *t)
{
    if (this->owner != t) {
        return nullptr;
    } 
    OWNER *o = this->owners.removeFirst();
    if (o != nullptr) {
        this->owner = o->getOwner();
    }
    return o;
}

bool MUTEX::hasLock ()
{
    return !this->owners.isEmpty();
}




MUTEX_FACTORY::MUTEX_FACTORY ()
{
    this->freeMutexCount = MAX_MUTEX_COUNT;
    this->freeOwnersCount = MAX_OWNERS_COUNT;
}

MUTEX_FACTORY::~MUTEX_FACTORY ()
{
    
}

void MUTEX_FACTORY::init ()
{
    
}

arch_word_t MUTEX_FACTORY::lock (struct vm_thread_desc *t, arch_word_t id)
{
    OWNER *o = nullptr;
    MUTEX *m = this->mutexList.getFirst();
    while (m != nullptr) {
        if (m->Id == id) {
            o = this->allocOwner();
            if (o == nullptr) {
                return MUTEX_SMALL_CORE;
            }
            o->init(t);
            return m->lock(o);
        }
        m = m->next();
    }
    m = this->allocMutex ();
    if (m == nullptr) {
        return MUTEX_SMALL_CORE;
    }
    m->init(id);
    this->mutexList.addLast(m);
    return m->lock(t);
}



vm_thread_desc *MUTEX_FACTORY::unlock (struct vm_thread_desc *t, arch_word_t id)
{
    OWNER *o;
    MUTEX *m = this->mutexList.getFirst();
    while (m != nullptr) {
        if (m->Id == id) {
            o = m->unlock(t);
            if (o != nullptr) {
                t = o->getOwner();
                this->freeOwner(o);
                return t;
            } else {
                this->mutexList.remove(m);
                this->freeMutex(m);
                return nullptr;
            }
        }
        m = m->next();
    }
    return nullptr;
}

arch_word_t MUTEX_FACTORY::monitor_start (arch_word_t mon_id, arch_word_t *mutexList)
{
    return MUTEX_GRANT_UNLOCK;
}

arch_word_t MUTEX_FACTORY::monitor_stop (arch_word_t mon_id, arch_word_t *mutexList)
{
    return MUTEX_GRANT_UNLOCK;
}

OWNER *MUTEX_FACTORY::allocOwner ()
{
    if (this->freeOwnersCount == 0) {
        return nullptr;
    }
    this->freeOwnersCount--;
    for (arch_word_t i = 0; i < MAX_OWNERS_COUNT; i++) {
        if (this->ownerMemory[i].busy == false) {
            this->ownerMemory[i].busy = true;
            return &this->ownerMemory[i];
        }
    }
    return nullptr;
}

void MUTEX_FACTORY::freeOwner (OWNER *o)
{
    this->freeOwnersCount++;
    o->busy = false;
    o->owner = nullptr;
}

MUTEX *MUTEX_FACTORY::allocMutex ()
{
    if (this->freeMutexCount == 0) {
        return nullptr;
    }
    this->freeMutexCount--;
    for (arch_word_t i = 0; i < MAX_MUTEX_COUNT; i++) {
        if (this->mutexMemory[i].busy == false) {
            this->mutexMemory[i].busy = true;
            return &this->mutexMemory[i];
        }
    }
    return nullptr;
}

void MUTEX_FACTORY::freeMutex (MUTEX *m)
{
    this->freeMutexCount++;
    m->busy = false;
    m->owner = nullptr;
    m->Id = 0;
    m->owners.removeAll();
}

arch_word_t MUTEX_FACTORY::getFreeOwners ()
{
    return this->freeOwnersCount;
}

arch_word_t MUTEX_FACTORY::getFreeMutexs ()
{
    return this->freeMutexCount;
}


_EXTERN void *valloc (UINT32_T size);
_EXTERN void vfree (void *p);

