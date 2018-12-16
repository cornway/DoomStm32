#ifndef STACK_CLASS
#define STACK_CLASS
#include "memory_template.h"
#include "arrayList.h"

template <typename Object>
    class Stack : public Allocator<Object> {
        private :
            Object *array;
            int32_t pushIndex;
            int32_t length;
        
        public :
            Stack ()
            {
                
            }
            ~Stack ()
            {
                this->Delete(array);
                this->Delete(this);
            }
            Stack (uint32_t size)
            {
                array = this->NewlArray(size);
                length = size;
                pushIndex = 0;
            }
            void operator () (uint32_t size)
            {
                array = this->NewlArray(size);
                length = size;
                pushIndex = 0;
            }
            
            
            Object *push (Object &o)
            {
                if (pushIndex >= length) {
                    return nullptr;
                }
                array[pushIndex++] = &o;
                return &o;
            }
            Object *push (Object *o)
            {
                return push(*o);
            }
            
            Object *pop ()
            {
                if (pushIndex == 0) {
                    return nullptr;
                }
                return array[--pushIndex];
            }
            void clear ()
            {
                pushIndex = 0;
            }
            
            
            ArrayList<Object> *clone ()
            {
                ArrayList<Object> *listT = this->New();
                for (int i = 0; i < length; i++) {
                    listT->addFirst(array[i]);
                }
                return listT;
            }
            
            bool isEmpty ()
            {
                if (pushIndex == 0) {
                    return true;
                }
                return false;
            }
            bool isFull ()
            {
                if (pushIndex >= length) {
                    return true;
                }
                return false;
            }
        
    };






#endif


/*End of file*/

