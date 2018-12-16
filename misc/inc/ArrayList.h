#ifndef ARRAY_LIST_CPP
#define ARRAY_LIST_CPP

#include <stdint.h>


template <typename List, typename Object>
	class DefaultArrayListNode {
        public :
        DefaultArrayListNode (Object *object)
		{
            this->object = object;
		}    
		/*Default constructor*/
		/*Default interface to compare objects*/
		bool equals (DefaultArrayListNode<List, Object> &node)
		{
			//return this->object->equals(node->getUserObject());
            return true;
		}
        Object *getUserObject ()
        {
            return this->object;
        }
		Object *object;
		DefaultArrayListNode<List, Object> *prevLink;
	    DefaultArrayListNode<List, Object> *nextLink;
        
        
		/*Object defined interface*/
        //friend class List;
        
		private:
	};
    
template <typename Vector, typename Object>
    class Iterator {
            DefaultArrayListNode <Vector, Object> *node;
            Iterator (); 
            void operator () (DefaultArrayListNode <Vector, Object> &node);       
        public :
            bool hasNext ();  
            bool hasThis ();        
            void next ();
            Object *get  ();

    };

	template <typename Object>
		class ArrayList {
			public: 
			ArrayList ()
			{
				this->firstNode = nullptr;
				this->lastNode = nullptr;
			}
			
			/*put new item without compare - as result create an unsorted list*/
			
			Object &add (Object &o)
			{
                DefaultArrayListNode<ArrayList<Object>, Object> *node = (DefaultArrayListNode<ArrayList<Object>, Object> *) \
                            new DefaultArrayListNode<ArrayList<Object>, Object>(&o);
                (*node)(o);
				this->addBase(*node);
        return o;
			}
            
			Object &add (Object *o)
			{
				return this->add(*o);
			}
			
			Object &addFirst (Object &o)
			{
				DefaultArrayListNode<ArrayList<Object>, Object> *node = (DefaultArrayListNode<ArrayList<Object>, Object> *) \
                            new DefaultArrayListNode<ArrayList<Object>, Object>(&o);
				this->addFirstBase(*node);
        return o;
			}
			
			Object &addFirst (Object *o)
			{
				return this->addFirst(*o);
			}
			
			Object &addLast (Object &o)
			{
				DefaultArrayListNode<ArrayList<Object>, Object> *node = (DefaultArrayListNode<ArrayList<Object>, Object> *) \
                            new DefaultArrayListNode<ArrayList<Object>, Object>(o);
				this->addLastBase(node);
                return o;
			}
            
			Object &addLast (Object *item)
			{
				return this->addLast(*item);
			}
            
            Object *getFirst ()
            {
                return this->firstNode->getUserObject();
            }
			Object *getLast ()
            {
                return this->lastNode->getUserObject();
            }
			/*in this method all items will migrate from one list to another, with
			rewriting their links;
			*/
			ArrayList<Object> &addAll (ArrayList<Object> &arrayList)
			{
				while (arrayList.isEmpty() == false) {
					this->add(arrayList.removeFirst());
				}
				return *this;
			}
			
			template <typename Collection>
			ArrayList<Object> &addAll (Collection collection)
			{
				
				return *this;
			}
			
			
			Object *remove (Object &o)
			{
                DefaultArrayListNode<ArrayList<Object>, Object> *iterator = this->firstNode;
                while (iterator != nullptr && (iterator->getUserObject() != &o)) {
                    iterator = iterator->nextLink;
                }
                if (iterator != nullptr) {
                    this->removeBase(*iterator);
                }
                delete iterator;
                return &o;
			}
            
			Object *remove (Object *object)
			{
				return this->remove(*object);
			}
			
			Object *removeFirst ()
			{
                delete this->firstNode;
				return this->removeBase(this->firstNode)->getUserObject();
			}
			
			Object *removeLast ()
			{
                delete this->lastNode;
				return this->removeBase(this->lastNode)->getUserObject();
			}
			
			void removeAll ()
			{
				while (this->isEmpty() == false) {
                    this->removeFirst();
                }
			}
			
			
			
			 
             Iterator<ArrayList<Object>, Object> *iterator ()
             {
                static Iterator<ArrayList<Object>, Object> i;
                i(*this->firstNode);
                return &i;
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
                DefaultArrayListNode<ArrayList<Object>, Object> &addBase (DefaultArrayListNode<ArrayList<Object>, Object> &node)
                {
                    this->elementCount++;
                    DefaultArrayListNode<ArrayList<Object>, Object> *i,*j;
                    if (this->firstNode == nullptr) {
                        this->firstNode = &node;
                        this->lastNode = &node;
                        node.nextLink = nullptr;
                        node.prevLink = nullptr;
                        return node;
                    }
                    i = this->firstNode;
                    j = nullptr;
                    while (i != nullptr) {
                        if (i->equals(node) == true) {
                            j = i;
                            i = i->nextLink;
                            continue;
                        }
                        if (i->prevLink != nullptr) {
                            i->prevLink->nextLink = &node;
                            node.nextLink = i;
                            node.prevLink = i->prevLink;
                            i->prevLink = &node;
                            return node;
                        }
                        node.nextLink = i;
                        node.prevLink = nullptr;
                        i->prevLink = &node;
                        this->firstNode = &node;
                        return node;
                    }
                    j->nextLink = &node;
                    node.nextLink = nullptr;
                    this->lastNode = &node;
                    node.prevLink = j;
                    return node;					
                }  
                DefaultArrayListNode<ArrayList<Object>, Object> *addFirstBase (DefaultArrayListNode<ArrayList<Object>, Object> &node)
                {
                        this->elementCount++;
                        DefaultArrayListNode<ArrayList<Object>, Object> *i;
                        if (this->firstNode == nullptr) {
                            this->firstNode = &node;
                            this->lastNode = &node;
                            node.nextLink = nullptr;
                            node.prevLink = nullptr;
                            return &node;
                        }
                        i = this->firstNode;
                        node.nextLink = i;
                        node.prevLink = nullptr;
                        i->prevLink = &node;
                        this->firstNode = &node;
                        return &node;				
                }
                DefaultArrayListNode<ArrayList<Object>, Object> *addLastBase (DefaultArrayListNode<ArrayList<Object>, Object> *node)
                {
                    this->elementCount++;
                    DefaultArrayListNode<ArrayList<Object>, Object> *i;
                    if (this->firstNode == nullptr) {
                        this->firstNode = node;
                        this->lastNode = node;
                        node->nextLink = nullptr;
                        node->prevLink = nullptr;
                        return node;
                    }
                    i = this->lastNode;
                    node->prevLink = i;
                    node->nextLink = nullptr;
                    i->nextLink = node;
                    this->lastNode = node;			
                    return node;
                }
                DefaultArrayListNode<ArrayList<Object>, Object> *removeBase (DefaultArrayListNode<ArrayList<Object>, Object> *node)
                {
                    if (this->elementCount <= 0){
                        return node;
                    }
                    this->elementCount--;
                    DefaultArrayListNode<ArrayList<Object>, Object> *l = node->prevLink,*r = node->nextLink;
                    if (!l&&!r) {
                        this->firstNode = nullptr;
                        this->lastNode = nullptr;
                        return node;
                    }
                    if (!l) {
                        this->firstNode = r;
                        r->prevLink = nullptr;
                    }   else    {
                        l->nextLink = r;
                    }
                    if (!r) {
                    l->nextLink = nullptr;
                    this->lastNode = l;
                    }   else    {
                        r->prevLink = l;
                    }	
                    if (node == nullptr) {
                        return firstNode;
                    }
                    return node;
                }    
                
                uint32_t elementCount;
                DefaultArrayListNode<ArrayList<Object>, Object> *firstNode;
                DefaultArrayListNode<ArrayList<Object>, Object> *lastNode;
		};
                          
        template <typename V, typename o>
        Iterator<V, o>::Iterator ()
        {
            
        }
        template <typename V, typename o>
        void Iterator<V, o>::operator () (DefaultArrayListNode<V, o> &node)
        {
            this->node = &node;
        }
        template <typename V, typename o>
        bool Iterator<V, o>::hasNext ()
        {
            if (node->nextLink != nullptr) {
                return true;
            } else {
                return false;
            }
        }
        template <typename V, typename o>
        bool Iterator<V, o>::hasThis ()
        {
            if (node != nullptr) {
                return true;
            } else {
                return false;
            }
        }
        template <typename V, typename o>
        void Iterator<V, o>::next()
        {
            node = node->nextLink;
        }
        template <typename V, typename o>
        o *Iterator<V, o>::get()
        {
            return node->getUserObject();
        }
#endif /*ARRAY_LIST_CPP*/

/*End of file*/


