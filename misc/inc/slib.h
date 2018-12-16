#ifndef STRING_PROCESSING_LIBRARY
#define STRING_PROCESSING_LIBRARY

#include <stdint.h>
#include <string.h>
#include <ctype.h>

namespace slib {
	typedef char char_t;
	
	
	enum ERRORS {
		
	};
	
	enum COMP_RESULT {
		MATCH 			= 1,
		NO_MATCH 		= -1,
		BREAK_FIRST 	= 1,
		BREAK_SECOND 	= 2,
		OUT_OF_BOUNDS 	= 3,
	};
	
	COMP_RESULT compare (char_t *s1, char_t *s2, int size, char_t end);
	COMP_RESULT compare (char_t *s1, char_t *s2);
	
	char_t *subString (char_t *dest, const char_t *src, int start, int end);
	template <typename Collector>
	void subString (Collector collector, char_t *s, int start, int end);
	
} /*namespace slib*/

#endif /*STRING_PROCESSING_LIBRARY*/

/**/

