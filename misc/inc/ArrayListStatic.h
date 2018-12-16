#ifndef ARRAY_LIST_BASE
#define ARRAY_LIST_BASE
#include <stdint.h>


#define FOREACH_ARRAY_LIST(type, array, name) \
for (type *name = array.getfirst(); name != nullptr; name = name->next())
	
	template <typename Object>
		class ArrayListBase;
	
	template <typename O>
		class Link {
			private :
				O *nextLink, *prevLink;
			public :
				Link () : nextLink(nullptr), prevLink(nullptr)
				{
					
				}
				
				O *next ()
				{
					return nextLink;
				}
				
				O *prev ()
				{
					return prevLink;
				}
				
				virtual bool equals (Link<O> *node)
				{
					return false;
				}
				
				friend class ArrayListBase<O>;
		};

	template <typename Object>
		class ArrayListBase {
			public: 
			ArrayListBase ()
			{
				this->firstNode = nullptr;
				this->lastNode = nullptr;
                elementCount = 0;
			}
			
			/**/
			Object *add (Object *item)
			{
				this->elementCount++;
                    Object *i,*j;
                    if (this->firstNode == nullptr) {
                        this->firstNode = item;
                        this->lastNode = item;
                        item->nextLink = nullptr;
                        item->prevLink = nullptr;
                        return item;
                    }
                    i = this->firstNode;
                    j = nullptr;
                    while (i != nullptr) {
                        if (i->equals(item) == true) {
                            j = i;
                            i = i->nextLink;
                            continue;
                        }
                        if (i->prevLink != nullptr) {
                            i->prevLink->nextLink = item;
                            item->nextLink = i;
                            item->prevLink = i->prevLink;
                            i->prevLink = item;
                            return item;
                        }
                        item->nextLink = i;
                        item->prevLink = nullptr;
                        i->prevLink = item;
                        this->firstNode = item;
                        return item;
                    }
                    j->nextLink = item;
                    item->nextLink = nullptr;
                    this->lastNode = item;
                    item->prevLink= j;
                    return item;					
			}
			
			Object *addFirst (Object *item)
			{
				this->elementCount++;
					Object *i;
					if (this->firstNode == nullptr) {
						this->firstNode = item;
						this->lastNode = item;
						item->nextLink = nullptr;
						item->prevLink = nullptr;
						return item;
					}
					i = this->firstNode;
					item->nextLink = i;
					item->prevLink = nullptr;
					i->prevLink = item;
					this->firstNode = item;
					return item;
			}
			
			
			Object *addLast (Object *item)
			{
				this->elementCount++;
				Object *i;
				if (this->firstNode == nullptr) {
					this->firstNode = item;
					this->lastNode = item;
					lastNode->nextLink = nullptr;
					firstNode->prevLink = nullptr;
					return item;
				}
				i = this->lastNode;
				item->prevLink = i;
				item->nextLink = nullptr;
				i->nextLink = item;
				this->lastNode = item;			
				return item;
			}		
            
            Object *getFirst ()
            {
                return this->firstNode;
            }
			Object *getLast ()
            {
                return this->lastNode;
            }
			/*in this method all items will migrate from one list to another, with
			rewriting their links;
			*/
			ArrayListBase<Object> *addAll (ArrayListBase<Object> &arrayList)
			{
				while (arrayList.isEmpty() == false) {
					this->add(arrayList.removeFirst());
				}
				return this;
			}
			
			template <typename Collection>
			ArrayListBase<Object> *addAll (Collection collection)
			{
				return this;
			}
			
			Object *remove (Object *object)
			{
				if (this->elementCount <= 0){
                    return object;
                }
				this->elementCount--;
				Object *l = object->prevLink,*r = object->nextLink;
				if (!l&&!r) {
                    this->firstNode = nullptr;
                    this->lastNode = nullptr;
                    return object;
                }
                if (!l) {
                    this->firstNode = r;
                    r->prevLink = nullptr;
                }   else    {
                    if (r)
                    l->nextLink = r;
                }
                if (!r) {
                    l->nextLink = nullptr;
                    this->lastNode = l;
                }   else    {
                    r->prevLink = l;
                }	
				return object;
            }
			
			Object *removeFirst ()
			{
				if (this->elementCount <= 0){
                    return nullptr;
                }
				this->elementCount--;
                Object *o = firstNode;
                
				if (!o->nextLink) {
                    this->firstNode = nullptr;
                    this->lastNode = nullptr;
                    return o;
                }
                firstNode = o->nextLink;
                firstNode->prevLink = nullptr;
	
				return o;
			}
			
			Object *removeLast ()
			{
				if (this->elementCount <= 0){
                    return nullptr;
                }
				this->elementCount--;
                Object *o = lastNode;
                
				if (!o->prevLink) {
                    this->firstNode = nullptr;
                    this->lastNode = nullptr;
                    return o;
                }
                lastNode = o->prevLink;
                lastNode->nextLink = nullptr;
	
				return o;
			}
			
			void removeAll ()
			{
				while (this->isEmpty() == false) {
                    this->removeFirst();
                }
			}
			
			template <typename L>
			void foreach (L l)
			{
				Object *o = this->firstNode;
				Object *s = o;
				while (o != nullptr) {
					s = o;
					o = o->nextLink;
					l(s);
				}
			}
			
			
			Object *
			get (int id)
			{
                if (id > this->elementCount) {
                    return nullptr;
                }
				Object *o = this->firstNode;
				while (--id > 0) {
					o = o->nextLink;
				}
				return o;
			}
			
			
			bool isEmpty ()
			{
				if (this->elementCount > 0) {
					return false;
				} else {
					return true;
				}
			}
			
			uint32_t size ()
			{
				return this->elementCount;
			}
			private:
			uint32_t elementCount;
			Object *firstNode;
			Object *lastNode;
		};
#endif /*ARRAY_LIST_BASE*/

/*End of file*/

