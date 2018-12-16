#ifndef MUTEX_H
#define MUTEX_H

#include "vm_conf.h"
#include "arch.h"
#include "thread.h"
#include "iterable.h"

#ifndef MAX_OWNERS_COUNT
#define MAX_OWNERS_COUNT    (8U)
#endif

#ifndef MAX_MUTEX_COUNT
#define MAX_MUTEX_COUNT     (8U)
#endif


enum {
    MUTEX_GRANT_LOCK,
    MUTEX_GRANT_WAIT,
    MUTEX_GRANT_UNLOCK,
    MUTEX_SMALL_CORE,
};



class MUTEX_FACTORY;


class MUTEX;

class OWNER : public Link<OWNER> {
    private :
        struct vm_thread_desc *owner;
        bool busy;
        
        friend class MUTEX_FACTORY;
    public :
        OWNER ();
        OWNER (struct vm_thread_desc *owner);
        OWNER (OWNER &owner);
    
        void init (struct vm_thread_desc *owner);
        
        struct vm_thread_desc *getOwner (); 
        void operator = (OWNER &owner);
        void operator = (OWNER *owner);
    
        ~OWNER ();
};

class MUTEX : public Link<MUTEX> {
    private :
        vector::Vector<OWNER> owners;
        struct vm_thread_desc *owner;
        arch_word_t Id;
        bool busy;
    
        friend class MUTEX_FACTORY;
    public :
        MUTEX ();
        ~MUTEX ();
    
        void init (arch_word_t id);
        arch_word_t lock (struct vm_thread_desc *t);
        arch_word_t lock (OWNER *owner);
        OWNER *unlock (struct vm_thread_desc *t);
    
        arch_word_t getId ();
        bool hasLock ();
};


class MUTEX_FACTORY {
    private :
        MUTEX mutexMemory[MAX_MUTEX_COUNT];
        OWNER ownerMemory[MAX_OWNERS_COUNT];
        
        uint16_t freeOwnersCount;
        uint16_t freeMutexCount;
    
        vector::Vector<MUTEX> mutexList;
    
        OWNER *allocOwner();
        void freeOwner (OWNER *owner);
    
        MUTEX *allocMutex ();
        void freeMutex (MUTEX *mutex);
    public :
        MUTEX_FACTORY ();
        ~MUTEX_FACTORY();
    
        void init ();
        arch_word_t lock (struct vm_thread_desc *t, arch_word_t id);
        struct vm_thread_desc *unlock (struct vm_thread_desc *t, arch_word_t id);
        arch_word_t monitor_start (arch_word_t mon_id, arch_word_t *mutexList);
        arch_word_t monitor_stop (arch_word_t mon_id, arch_word_t *mutexList);
    
        arch_word_t getFreeOwners ();
        arch_word_t getFreeMutexs ();
        
};



#define     MON_ID  (0xFFFFFFFE)

/*
class MONITOR_FACTORY;
class MONITOR;

class KEY : public Link<KEY> {
    private :
        MUTEX *mutex;
        arch_word_t id;
        KEY ();
        ~KEY();
    public :
        
};

class MONITOR : public Link<MONITOR> {
    private :
        vector::Vector<KEY> keys;
        arch_word_t id;
        MONITOR ();
        ~MONITOR ();
    public :
        
        
};


MONITOR_FACTORY {
    
};
*/
#endif /*MUTEX_H*/


