#ifndef EXCEPTION_CLASS 
#define EXCEPTION_CLASS

#include<stdint.h>

namespace exception {

enum ExceptionParameters {
	MESSAGE_LENGTH = 24,
};
	
class Throwable {
	private :
		char message[MESSAGE_LENGTH];
	public :
		Throwable (char *mess)
		{
			int index = 0;
			while (mess[index] != '\0') {
				message[index] = mess[index];
				index++;
				if (index >= MESSAGE_LENGTH - 1) {
					message[index + 1] = '\0';
					throw new Throwable((char *)"Too Large !");
				}
			}
			message[index] = '\0';
		}
		~Throwable ()
		{
			delete(this);
		}
		
		char *getMessage ()
		{
			return message;
		}
};

template <class Handler>
class Exception : public Throwable {
private:
	Handler *native;
public :
	Exception (char *message, Handler *native) : Throwable (message)
	{
		this->native = new Handler(native);
	}
	
	virtual void handle ()
	{
		
	}
};

}

#endif /*EXCEPTION_CLASS*/

/*End of file*/

