#ifndef QUEUE_CLASS
#define QUEUE_CLASS



template <typename Object, int Capacity>
class QueueStatic {
	private :
		Object array[Capacity];
		int tailPoint, headPoint;
		int quantity;
		
		void __push (Object &o)
		{
			if (headPoint + 1 == tailPoint || (headPoint + 1 == Capacity && tailPoint == 0)) {
                return;
            }
            array[headPoint] = o;
            quantity++;
            headPoint++;
            if (headPoint == Capacity) {
                headPoint = 0;
            }
		}
		
		Object &__pop ()
		{
			if (tailPoint == Capacity) {
                tailPoint = 0;
            }
            if (tailPoint == headPoint) {
                return array[0];
            }  
            tailPoint++;
            quantity--;
            return array[tailPoint - 1];
		}
	
	public :
		QueueStatic ()
		{
			this->tailPoint = 0;
			this->headPoint = 0;
			this->quantity = 0;
		}
		
		void push (Object o)
		{
			this->__push(o);
		}
		void push (Object &o)
		{
			this->__push(o);
		}
		void push (Object *o)
		{
			this->__push(*o);
		}
		
		
		Object &pop ()
		{
			return this->__pop();
		}
		
		int size ()
		{
			return this->quantity;
		}
	
};


#endif /*QUEUE_CLASS*/

/*End of file*/


