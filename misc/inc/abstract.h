#ifndef __ABSTRACT__
#define __ABSTRACT__

#include "iterable.h"
#include "string.h"

namespace abstract {

    class Event {
        private :
            uint32_t cause;
            void *source;
        public :
            Event (void *source, uint32_t cause = 0)
            {
                this->cause = cause;
                this->source = source;
            }

            void *getSource ()
            {
                return source;
            }

            uint32_t getCause ()
            {
                return cause;
            }
    }; /*class Event*/

    class EventBurner;

    class EventListener : public Link<EventListener> {
        private :
            void (*callback) (Event e);
            int fire (Event e)
            {
                callback(e);
                return 0;
            }
        public :
            EventListener ( void (*l) (Event e) ) : callback(l) {}

            friend class EventBurner;

    }; /*EventListener*/

    class EventBurner {
        private :
            vector::Vector<EventListener> list;
        public :
            EventBurner () : list ()
            {
                
            }
            ~EventBurner () 
            {
                
            }

            void addListener (EventListener *l)
            {
                list.addFirst(l);
            }
            
            void removeAll ()
            {
                EventListener *l = this->list.getFirst();
                EventListener *ln = l->next();
                while (l != nullptr) {
                    this->list.remove(l);
                    delete l;
                    l = ln;
                    ln = ln->next();
                }
            }

            void fireEvents (Event e)
            {
                list.foreach(
                    [&] (EventListener *l) {
                        l->fire(e);
                });
            }
    }; /*class EventBurner*/

    class Thread {
        private :
            void (*thread) (char* args);
        public :
            template <typename L>
            Thread (L l) 
            {
                this->thread = l;
            }

            void join (char *args)
            {
                (*this->thread)(args);
            }
    }; /*class Thread*/

}; /*namespace abstract*/

#endif /*__ABSTRACT__*/

/**/


