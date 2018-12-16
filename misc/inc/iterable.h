#ifndef ITERABLE_COLLECTION
#define ITERABLE_COLLECTION

#include "ArrayListStatic.h"

namespace vector {
	
	template <typename Item>
		class Vector;
	
	template <typename Item>
		class Iterator {
			private :
				Vector<Item> *vector;
				Item *item;
			public :
				Iterator (Vector<Item> *v, Item *i) : vector( v ), item( i )
				{
					
				}
				
				bool
				operator != (Iterator<Item> &it) const
				{
					return it.item != item;
				}
				
				Item *
				operator * () const 
				{
					return item;
				}
				
				Iterator<Item> &operator ++ ()
				{
					item = item->next();
					
					return *this;
				}
				
		}; /*class Iterator*/
		
		
		template <typename Item>
			class Vector : public ArrayListBase<Item>{
			private :
				
			public :
				Vector () : ArrayListBase<Item>() {}
			
				Iterator<Item> begin ()
				{
					return Iterator<Item>(this, this->getFirst());
				}
				
				Iterator<Item> end ()
				{
					return Iterator<Item>(this, this->getLast());
				}
		};
			
		
		
	
}; /*namespace vector*/

namespace collect {
	
	template <typename Item>
		class Collection;
	
	
	/*linked node does not create a new instance of the object, just hold a link to an existing*/
	template <typename Item>
	class LinkedNode : public Link<LinkedNode<Item> >{
		private :
			Item *item;
		public :
			LinkedNode (Item &i)
			{
				item = &i;
			}
			
			LinkedNode (Item *i)
			{
				item = i;
			}
			
			Item&
			getUserObject ()
			{
				return *item;
			}
			
			bool 
			compare (Item &i)
			{
				if (*item == i) {
					return true;
				} else {
					return false;
				}
			}
			
			bool
			compare (LinkedNode<Item> &node)
			{
				if (item == &node.getUserObject()) {
					return true;
				} else {
					return false;
				}
			}
	};
	
	template <typename Node>
	class Iterator {
		private :
			Node *node;
		public :
			Iterator (Node *i) : node( i )
			{
				
			} 
			
			bool 
			operator != (Iterator<Node> &i) const
			{
				return i.node != node;
			}
			
			Node *
			operator * () const
			{
				return node;
			}
			
			Iterator<Node> &
			operator ++ ()
			{
				node = node->next();
				
				return *this;
			}	
	}; /*class Iterator*/
	
	template <typename Item>
		class Collection {
			private :
				vector::Vector<LinkedNode<Item> > vector;
			public:
				Collection () {}
					
				Iterator<LinkedNode<Item> >
				begin ()
				{
					return Iterator<LinkedNode<Item> >(vector.getFirst());
				}
				
				Iterator<LinkedNode<Item> >
				end() 
				{
					return Iterator<LinkedNode<Item> >(vector.getLast());
				}
				
				void
				add (Item &item)
				{
					vector.add(new LinkedNode<Item>(item));
				}
				
				void
				remove (Item &item)
				{
					for (auto i : vector) {
						if (i.compare(item)) {
							this->remove(i);
							break;
						}
					}
				}
				
				void
				remove (LinkedNode<Item> &node)
				{
					for (auto i : vector) {
						if (i.compare(node)) {
							this->remove(i);
							break;
						}
					}
				}
				
				void
				remove (Item *item)
				{
					for (auto i : this) {
						if (i.compare(*item)) {
							this->remove(i);
							break;
						}
					}
				}
				
		}; /*class Collection*/
	
}; /*namespace collect*/


namespace array {
	
	template <typename Item>
		class Array;
	
	template <typename Item>
		class Iterator {
			private :
				Array<Item> *array;
				Item item;
				int index;
			public :
				Iterator (Array<Item> *a, Item i) : array(a), item(i)
				{
					index = 0;
				}
				
				Item
				operator * () const;	
				
				bool
				operator != (Iterator<Item> &i) const
				{
					return i.item != item;
				}
				
				Iterator<Item> &
				operator ++ ()
				{
					index++;
					
					return *this;
				}
				
		}; /*class Iterator*/
		
		template <typename Item>
		class Array {
			private :
				Item *array;
				int index;
				int size;
			public :
				Array (int size) : index(0), size(size == 0 ? 1 : size) 
				{
					this->array = new Item[size];
				}
				
				Array (vector::Vector<Item> &v) : index(0)
				{
					this->size = v.size();
					this->array = new Item[this->size];
					for (auto item : v) {
						this->push(*item);
					}
				}
				
				~Array () 
				{
					delete(this->array);
				}
                
                template <typename L>
                void
                foreach (L l)
                {
                    for (int i = 0; i < this->index; i++) {
                        l(this->array[i]);
                    }
                }
					
				Iterator<Item> 
				begin () 
				{
					return Iterator<Item>(this, array[0]);
				}
				
				Iterator<Item>
				end () 
				{
					return Iterator<Item>(this, array[size - 1]);
				}
				
				void
				put (Item &item, int i)
				{
					if (i < this->size) {
						array[i] = item;
					}
				}
				
				Item
				get (int i)
				{
					if (i < this->size && i >= 0) {
						return array[i];
					}
                    return array[0];
				}
				
				void
				push (Item &i)
				{
					if (index >= this->size) {
						return;
					}
					array[index] = i;
					index++;
				}
				
				Item 
				pop ()
				{
					if (index < 0) {
						index = 0;
						return array[0];
					} else {
						return array[index--];
					}
				}
                
                int
                getSize ()
                {
                    return this->index;
                }
				
				
		}; /*class Array*/
		
		template <typename Item>
		Item Iterator<Item>::operator * () const
		{
			return array->get(index);
		}
	
}; /*namespace array*/


namespace map {
	
	template <typename Key, typename Value>
	class Map;
	
	template <typename Key, typename Value>
	class Element;
	
	template <typename Key, typename Value>
		class MapValue : public Link<MapValue<Key, Value> > {
		private :
			Value value;
			
			MapValue (Value &value) : value(value)
			{
				
			}
			
			Value &
			getValue ()
			{
				return this->value;
			}
			
			~MapValue ()
			{
				
			}
			
			friend class Map<Key, Value>;
			friend class Element<Key, Value>;
		public :
			
	}; /*class MapValue*/
	
	template <typename Key, typename Value>
	class Element : public Link<Element<Key, Value> > {
		private :
			Key key;
			vector::Vector<MapValue<Key, Value> > valuesArray;
			
			Element (Key &key) : key(key), valuesArray()
			{
				
			}
			
			void
			add (Value value)
			{
				this->valuesArray.add( new MapValue<Key, Value>(value) );
			}
			
			~Element ()
			{
				auto v = this->valuesArray.getFirst();
				auto r = v;
				while (v != nullptr) {
					r = v;
					v = v->next();
					valuesArray.remove(r);
					(*r).~MapValue<Key, Value>();
					delete(r);
				}
			}
			
			vector::Vector<MapValue<Key, Value> > &
			getValuesArray ()
			{
				return this->valuesArray;
			}
			
			friend class Map<Key, Value>;
		public :
			
			Key &
			getKey ()
			{
				return this->key;
			}
			
			Value &
			getValue (int num)
			{
				return this->valuesArray.get(num);
			}
			

	};/*class Element*/
	
	template <typename Key, typename Value>
	class Iterator {
		private :
			
		public :
			Iterator () 
			{
				
			}
	}; /*class Iterator*/
	
	
	template <typename Key, typename Value>
	class Map {
		private :
			vector::Vector<Element<Key, Value> > elements;
		public :
			Map () : elements()
			{
				
			}
			
			~Map ()
			{
				auto e = this->elements.getFirst();
				auto r = e;
				while (e != nullptr) {
					r = e;
					e = e->next();
					elements.remove(r);
					(*r).~Element<Key, Value>();
					delete(r);
				}
			}
			
			void
			set (Key key, Value value)
			{
				for (auto element : this->elements) {
					if (element->getKey() == key) {
						element->add(value);
						return;
					}
				}
				auto element = new Element<Key, Value>(key);
				element->add(value);
				this->elements.add(element);
			}
			
			Value &
			get (Key &key, int num = 0)
			{
				for (auto element : this->elements) {
					if (element->getKey() == key) {
						return element->getValue(num);
					}
				}
                return this->elements[0]->getValue(0);
			}
			
			Value &
			get (Key key, int num = 0)
			{
				for (auto element : this->elements) {
					if (element->getKey() == key) {
						return element->getValue(num);
					}
				}
                return this->elements[0]->getValue(0);
			}
			
			array::Array<Value>  
			getArray (Key &key)
			{
				for (auto element : this->elements) {
					if (element->getKey() == key) {
						array::Array<Value> array( element->getValuesArray().size() );
						for (auto v : element->getValuesArray()) {
							array.push(v);
						}
						return array;
					}
				}
				return array::Array<Value>(1);
			}
			
			array::Array<Value>  
			getArray (Key key)
			{
				for (auto element : this->elements) {
					if (element->getKey() == key) {
						array::Array<Value> array( element->getValuesArray().size() );
						for (auto v : element->getValuesArray()) {
							array.push( v->getValue() );
						}
						return array;
					}
				}
				return array::Array<Value>(1);
			}
	}; /*class Map*/
}; /*namspace map*/

#endif /*ITERABLE_COLLECTION*/

/**/

