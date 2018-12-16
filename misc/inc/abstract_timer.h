#ifndef ABSTRACT_TIMER_CLASS
#define ABSTRACT_TIMER_CLASS

#include "ArraylistStatic.h"

template <typename C>
C *copyCharacters (C *dest, const C *src, int length = 0)
{
	int index = 0;
	if (length == 0) {
		while (src[index] != (C)'\0') {
			dest[index] = src[index];
			index++;
		}
	} else {
		while (src[index] != (C)'\0' && index < length) {
			dest[index] = src[index];
			index++;
		}
	}
	dest[index] = '\0';
	return dest;
}

template <typename C>
int compareCharSet (C *s1, C *s2, int length = 0)
{
	int index = 0;
	if (length < 0) {
		return -1;
	}
	if (length == 0) {
		while (s1[index] == s2[index]) {
			if (s1[index] == (C)'\0' || s2[index] == (C)'\0') {
				return 0;
			}
			index++;
		}
		return 1;
	} else {
		while (s1[index] == s2[index]) {
			if (s1[index] == (C)'\0' || s2[index] == (C)'\0' || index >= length) {
				return 0;
			}
			index++;
		}
		return 1;
	}
}

namespace abstract {

	#define ABSTRACT_TIMER_NAME_LENGTH (16)
	#define TIM_NONAME ("Unnamed timer ###\0")

	enum	TIMER_MODES {
		TIM_STOP = 0,
		TIM_RUN = 1,
	};

	class AbstractTimer : public DefaultArrayListBaseNode<AbstractTimer> {
		private :
			char name[ABSTRACT_TIMER_NAME_LENGTH];
			int (*callback) (AbstractTimer *);
			int8_t mode;
			uint32_t count;
			uint32_t compare;
		/*
			AbstractTimer ()
			{
				this->callback = nullptr;
				this->mode = TIM_STOP;
				this->count = 0;
				this->compare = 0;
				copyCharacters(this->name, TIM_NONAME);
			}
		*/
			AbstractTimer (int (*callback) (AbstractTimer *), uint32_t compare, char *name)
			{
				this->callback = callback;
				this->mode = TIM_STOP;
				this->count = 0;
				this->compare = compare;
				copyCharacters(this->name, name, ABSTRACT_TIMER_NAME_LENGTH);
			}
			~AbstractTimer () {}
			
			int tick ()
			{
				if (this->count == this->compare) {
					this->count = 0;
					(this->callback) (this);
					return 1;
				} else {
					this->count++;
				}
				return 0;
			}
			
			void reset ()
			{
				this->count = 0;
			}
		public :
			char *getName ()
			{
				return this->name;
			}
			
		friend class AbstractTimerFactory;
			
			
	};
	
	enum TIM_FACTORY_MODES {
		NORMAL = 0,
		REMOVE_WHEN_ELAPSE = 1,
	};
	
	class AbstractTimerFactory {
		private :
			ArrayListBase<AbstractTimer> timers;
			TIM_FACTORY_MODES mode;
		
		public :
			AbstractTimerFactory ()
			{
				this->mode = NORMAL;
			}
			~AbstractTimerFactory () {}
			
			int create (int (*callback) (AbstractTimer *), uint32_t compare, char *name)
			{
				/*validate name, if exist timer with the same name - return*/
				AbstractTimer *iterator = timers.getFirst();
				while (iterator != nullptr) {
					if (compareCharSet(name , iterator->name, ABSTRACT_TIMER_NAME_LENGTH) == 1) {
						return -1;
					}
					iterator = iterator->nextLink;
				}
				timers.add( new  AbstractTimer(callback, compare, name));
				return 1;
			}
			int replace (int (*callback) (AbstractTimer *), uint32_t compare, char *name)
			{
				/*validate name, if exist timer with the same name - return*/
				AbstractTimer *iterator = timers.getFirst();
				while (iterator != nullptr) {
					if (compareCharSet(name , iterator->name, ABSTRACT_TIMER_NAME_LENGTH) == 1) {
						iterator->callback = callback;
						iterator->compare = compare;
						return -1;
					}
					iterator = iterator->nextLink;
				}
				timers.add( new  AbstractTimer(callback, compare, name));
				return 1;
			}
			
			int remove (char *name)
			{
				AbstractTimer *iterator = timers.getFirst();
				while (iterator != nullptr) {
					if (compareCharSet(name , iterator->name, ABSTRACT_TIMER_NAME_LENGTH) == 1) {
						timers.remove(iterator);
						iterator->~AbstractTimer();
						delete(iterator);
						return 1;
					}
					iterator = iterator->nextLink;
				}
				return  -1;
			}
			
			int tick ()
			{
				AbstractTimer *removable = nullptr;
				AbstractTimer *iterator = timers.getFirst();
				while (iterator != nullptr) {
					removable = iterator;
					iterator = iterator->nextLink;
					if (this->mode = NORMAL) {
						removable->tick();
					} else {
						if (removable->tick()) {
							this->timers.remove(removable);
							iterator->~AbstractTimer();
							delete(iterator);
						}
					}
				}
				return  0;
			}
			
			
			
			void setMode (TIM_FACTORY_MODES mode)
			{
				this->mode = mode;
			}
	};

};/*namespace*/

#endif /*ABSTRACT_TIMER_CLASS*/


/*End of file*/

