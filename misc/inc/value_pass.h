#ifndef VALUE_PASS
#define VALUE_PASS

#include "stdint.h"
#include "time.h"

struct parse_result {
	int result;
	int start;
};

parse_result parse_dint (char *input);
char *insert (char *buf, const char *input, int32_t integer);
char *parse_date (char *buf, time::Time &time, time::Day &day, const char *format);
char *insert (char *buf, const char *dest,  const char *src, int pos);
char *insert (char *buf, const char *dest, const char *src);
int indexof (const char *input, const char *seq);

char *subString (char *buf, const char *input, int start, int end);
char *subString (char *buf, const char *input, int start, char end);
char *subString (char *buf, const char *input, char start, char end);


void enumeration (char *(*collector) (const char *, int, int), const char *input, char delimiter);

/*primitives*/
int isdigit (char ch);

#endif /*VALUE_PASS*/

/*End of file*/