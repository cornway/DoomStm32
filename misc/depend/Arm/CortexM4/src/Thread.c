#include "Thread.h"
#include <string.h>

_STATIC arch_word_t idle_thread_stack[VM_IDLE_THREAD_HEAP_SIZE / sizeof(arch_word_t)];
_STATIC vm_thread_list_t vm_thread_list_array[VM_THREAD_MAX_PRIORITY];
_STATIC struct vm_thread_desc *vm_thread_pool[THREAD_MAX_COUNT];
_STATIC vm_thread_list_t thread_drop_list;
_STATIC vm_thread_list_t thread_delay_list;
_STATIC vm_thread_list_t thread_fault_list;
_STATIC vm_thread_list_t thread_notify_list;
_STATIC vm_thread_list_t thread_cond_list;
_STATIC vm_thread_list_t thread_event_wait_list;
_STATIC vm_thread_list_t thread_mail_wait_list;
_STATIC arch_word_t thread_count;

_STATIC void thread_register (struct vm_thread_desc *thread_link); 
_STATIC void thread_unregister (struct vm_thread_desc *thread_link); 
_STATIC void thread_link (vm_thread_list_t *list, struct vm_thread_desc *thread_link);     /*link first to the list*/
_STATIC void thread_unlink (vm_thread_list_t *list, struct vm_thread_desc *thread_link);   /*unlink from list*/
_STATIC void _UNUSED(thread_move_list (vm_thread_list_t *dest, vm_thread_list_t *src));
_STATIC int test_thread (struct vm_thread_desc *t);
_STATIC UINT32_T threads_total;

_EXTERN void *valloc (UINT32_T size);
_EXTERN void vfree (void *p);   

_STATIC void thread_register (struct vm_thread_desc *t)
{
    for (INT32_T i = 0; i < THREAD_MAX_COUNT; i++) {
        if (vm_thread_pool[i] == NULL) {
            vm_thread_pool[i] = t;
            thread_count++;
            break;
        }
    }
}

_STATIC void thread_unregister (struct vm_thread_desc *t)
{
    for (INT32_T i = 0; i < THREAD_MAX_COUNT; i++) {
        if (vm_thread_pool[i] == t) {
            vm_thread_pool[i] = NULL;
            thread_count--;
            break;
        }
    }
}

void t_init_core ()
{
    thread_count = 0;
    for (INT32_T i = 0; i < THREAD_MAX_COUNT; i++) {
        vm_thread_pool[i] = NULL;
    }
    VM_THREAD_LHEAD_INIT(&thread_drop_list);
    VM_THREAD_LHEAD_INIT(&thread_delay_list);
    VM_THREAD_LHEAD_INIT(&thread_notify_list);
    VM_THREAD_LHEAD_INIT(&thread_cond_list);
    VM_THREAD_LHEAD_INIT(&thread_event_wait_list);
    VM_THREAD_LHEAD_INIT(&thread_mail_wait_list);
    
    for (arch_word_t i = 0; i < VM_THREAD_MAX_PRIORITY; i++) {
        VM_THREAD_LHEAD_INIT(&vm_thread_list_array[i]);
    }
}

int t_init (
                struct vm_thread_desc **t, 
                v_callback_t callback, 
                arch_byte_t priority, 
                arch_word_t id, 
                arch_word_t heap_size, 
                arch_byte_t privilege, 
                const char *name,
                arch_word_t arg_size,
                void *arg,
                arch_byte_t is_idle
           )
{
    if (callback == (v_callback_t)NULL) {
        return T_NULL_CALLBACK;
    }
    if (heap_size < VM_MIN_THREAD_HEAP_SIZE) {
        return T_SMALL_HEAP;
    }
    if (
        (privilege != CPU_ACCESS_LEVEL_0) &&
        (privilege != CPU_ACCESS_LEVEL_1) &&
        (privilege != CPU_ACCESS_LEVEL_2) &&
        (privilege != CPU_ACCESS_LEVEL_3) 
    ) {
        return T_PRIV_UNDEF;
    }
    if (priority >= VM_THREAD_MAX_PRIORITY) {
        return T_PRIOR_UNDEF;
    }
    arch_word_t l = strlen(name);
    if (l >= VM_DEF_THREAD_NAME_LEN) {
        return T_LONG_NAME;
    }
    if (is_idle) {
        *t = (struct vm_thread_desc *)idle_thread_stack;
    } else {
        *t = (struct vm_thread_desc *)valloc(heap_size + sizeof(struct vm_thread_desc));
        if (*t == (struct vm_thread_desc *)NULL) {
            return T_SMALL_CORE;
        }
    }

    memset(*t, sizeof(*t), 0);
    
    thread_register(*t);
    
    memcpy((*t)->name, name, l);
    
    (*t)->ID = id;
    (*t)->USE_FPU = THREAD_NO_FPU;
    (*t)->tdesc_stack_size = is_idle ? 
                                    sizeof(idle_thread_stack) - sizeof(struct vm_thread_desc) - sizeof(arch_word_t) : 
                                    heap_size;
    (*t)->PRIORITY = priority;
    (*t)->V_PRIORITY = priority;
    (*t)->PRIVILEGE = privilege;
    (*t)->faultMessage = "OK";
    (*t)->waitNotify = 0;
    (*t)->USE_FPU = THREAD_NO_FPU;

    (*t)->tdesc_stack_floor = ((arch_word_t)(*t) + sizeof(struct vm_thread_desc) + (*t)->tdesc_stack_size - STACK_ALLIGN) & (~(STACK_ALLIGN - 1));
    (*t)->CPU_FRAME = (CPU_STACK_FRAME *)((*t)->tdesc_stack_floor - sizeof(CPU_STACK));
    (*t)->CPU_FRAME->callControl.EXC_RET = (arch_word_t)callback;
    (*t)->CPU_FRAME->callControl.PC = (arch_word_t)callback;
    (*t)->CPU_FRAME->callControl.PSR = CPU_XPSR_T_BM;
    (*t)->CPU_FRAME->cpuStack.R0 = arg_size;
    (*t)->CPU_FRAME->cpuStack.R1 = (arch_word_t)arg;
    (*t)->CPU_FRAME->cpuStack.LR = (arch_word_t)VMBREAK;
    (*t)->cpuUsage = 0;

    return T_OK;
}

void t_destroy (struct vm_thread_desc *t)
{
    thread_unregister(t);
    vfree(t);
}


struct vm_thread_desc *t_ready (UINT32_T priority)
{
    struct vm_thread_desc *t;
    t = VM_THREAD_LIST_FIRST(&vm_thread_list_array[priority]);
    if (t == NULL) {
        return NULL;
    }
    while (t != NULL) {
        if (test_thread(t) == 0) {
            break;
        }
        t = t->nextLink;
    }
    return t;
}

void t_link_chain (struct vm_thread_desc *t, struct vm_thread_desc *l)
{
    thread_link(&t->chain, l);
}

void t_unlink_chain (struct vm_thread_desc *t, struct vm_thread_desc *l)
{
    thread_unlink(&t->chain, l);
}

void t_link_ready (struct vm_thread_desc *t)
{
    threads_total++;
    thread_link(&vm_thread_list_array[t->V_PRIORITY], t);
}

void t_unlink_ready (struct vm_thread_desc *t)
{
    threads_total--;
    thread_unlink(&vm_thread_list_array[t->V_PRIORITY], t);
}

void t_link_drop (struct vm_thread_desc *t)
{
    thread_link(&thread_drop_list, t);
}

void t_link_delay (struct vm_thread_desc *t)
{
    thread_link(&thread_drop_list, t);
}

void t_link_fault (struct vm_thread_desc *t)
{
    thread_link(&thread_fault_list, t);
}

void t_link_notify (struct vm_thread_desc *t)
{
    thread_link(&thread_notify_list, t);
}

void t_unlink_notify (struct vm_thread_desc *t)
{
    thread_unlink(&thread_notify_list, t);
}

struct vm_thread_desc *t_notify (const char *name)
{
    struct vm_thread_desc *t = thread_notify_list.firstLink;
    while (t != NULL) {
        if (strcmp(name, t->name) == 0) {
            return t;
        }
        t = t->nextLink;
    }
    return NULL;
}

void t_link_cond (struct vm_thread_desc *t)
{
    thread_link(&thread_cond_list, t);
}

arch_word_t t_check_cond (void (*linker) (struct vm_thread_desc *))
{
    if (thread_cond_list.elements <= 0) {
        return T_OK;
    }
    VM_THREAD_LFOREACH_SAFE(&thread_cond_list, t, tn) {
        if (t->link == NULL) {
            continue;
        } else {
            if ((t->cond)(0, NULL) == 0) {
                thread_unlink(&thread_cond_list, t);
                (linker)(t);
            }
        }
    }
    return T_OK;
}

void t_link_wait_event (struct vm_thread_desc *t)
{
    thread_link(&thread_event_wait_list, t);
}

arch_word_t t_fire_event (void (*linker) (struct vm_thread_desc *) ,const char *event_name)
{
    VM_THREAD_LFOREACH_SAFE(&thread_event_wait_list, t, tn) {
        if (strcmp(t->event_name, event_name) == 0) {
            thread_unlink(&thread_event_wait_list, t);
            (linker)(t);
        }
    }
    return T_OK;
}

int t_check_list ()
{
    return threads_total;
}

void t_refresh (void (*linker) (struct vm_thread_desc *))
{
    VM_THREAD_LFOREACH_SAFE(&thread_drop_list, t, tn) {
        t->V_PRIORITY = t->PRIORITY;
        thread_unlink(&thread_drop_list, t);
        linker(t);
    }
}

void t_tick (void (*linker) (struct vm_thread_desc *))
{
    VM_THREAD_LFOREACH_SAFE(&thread_delay_list, t, tn) {
        if (t->tdesc_wait_cnt) {
            t->tdesc_wait_cnt--;
        } else {
            thread_unlink(&thread_delay_list, t);
            linker(t);
        }
    }
}

struct vm_thread_desc *t_search (const char *name)
{
    for (arch_word_t i = 0; i < THREAD_MAX_COUNT; i++) {
        if (strcmp(name, vm_thread_pool[i]->name) == 0) {
            return vm_thread_pool[i];
        }
    }
    return NULL;
}

void t_unchain (void (*linker) (struct vm_thread_desc *), struct vm_thread_desc *t)
{
    VM_THREAD_LFOREACH_SAFE(&t->chain, t, tn) {
        thread_unlink(&t->chain, t);
        linker(t);
    }
}

void t_link_mail (struct vm_thread_desc *t)
{
    thread_link(&thread_mail_wait_list, t);
}

void t_unlink_mail(struct vm_thread_desc *t)
{
    thread_unlink(&thread_mail_wait_list, t);
}

struct vm_thread_desc *t_mail (const char *name)
{
    VM_THREAD_LFOREACH_SAFE(&thread_mail_wait_list, t, tn) {
        if (strcmp(name, t->name) == 0) {
            return t;
        }
    }
    return NULL;
}














_STATIC int test_thread (struct vm_thread_desc *t)
{
    return (t->fault || t->mutex || t->waitNotify);
}



_STATIC void thread_link (vm_thread_list_t *list, struct vm_thread_desc *t)
{
    list->elements++;
    struct vm_thread_desc *i;
    if (list->lastLink == NULL) {
        list->lastLink = t;
        list->firstLink = t;
        t->nextLink = NULL;
        t->prevLink = NULL;
        return;
    }
    i = list->lastLink;
    t->prevLink = i;
    t->nextLink = NULL;
    i->nextLink = t;
    list->lastLink = t;			
    return;
}

_STATIC void thread_unlink (vm_thread_list_t *list, struct vm_thread_desc *t)
{
    if (list->elements == 0){
        return;
    }
    list->elements--;
    struct vm_thread_desc *l = t->prevLink, *r = t->nextLink;
    if (!l && !r) {
        list->firstLink = NULL;
        list->lastLink = NULL;
        return;
    }
    if (!l) {
        list->firstLink = r;
        r->prevLink = NULL;
    }   else    {
        if (r)
        l->nextLink = r;
    }
    if (!r) {
        l->nextLink = NULL;
        list->lastLink = l;
    }   else    {
            r->prevLink = l;
    }	
}

_STATIC void thread_move_list (vm_thread_list_t *dest, vm_thread_list_t *src)
{
    struct vm_thread_desc *src_i = src->firstLink;
    struct vm_thread_desc *src_next_i = src_i->nextLink;
    while (src_i != NULL) {
        thread_unlink(src, src_i);
        thread_link(dest, src_i);
        src_i = src_next_i;
        src_next_i = src_next_i->nextLink;
    }
}

