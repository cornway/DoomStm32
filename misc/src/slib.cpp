#include "slib.h"

using slib;


COMP_RESULT compare (char_t *s1, char_t *s2, int size, char_t end)
{
	int index = 0;
	char_t c1, c2;
	while (index < size) {
		c1 = s1[index];
		c2 = s2[index];
		if (c1 == '\0') {
			return OUT_OF_BOUNDS;
		}
		if (c2 == '\0') {
			return OUT_OF_BOUNDS;
		}
		if (c1 != c2) {
			return NO_MATCH;
		}
		if (c1 == end) {
			return BREAK_FIRST;
		}
		if (c2 == end) {
			return BREAK_SECOND;
		}
		index++;
	}
	return MATCH;
}

COMP_RESULT compare (char_t *s1, char_t *s2)
{
	int index = 0;
	char_t c1 = '\0', c2 = '\0';
	while (true) {
		c1 = s1[index];
		c2 = s2[index];
		if (c1 == '\0') {
			return OUT_OF_BOUNDS;
		}
		if (c2 == '\0') {
			return OUT_OF_BOUNDS;
		}
		if (c1 != c2) {
			return NO_MATCH;
		}
		index++;
	}
	return MATCH;
}

char_t *subString (char_t *dest, const char_t *src, int start, int end)
{
	int size = end - start;
	int index = 0;
	if (size < 0) {
		return dest;
	}
	while (index < size) {
		if (src[index] == '\0') {
			break;
		}
		dest[index] = src[index + start];
		index++;
	}
	dest[index] = src[index];
}


template <typename Collector>
void subString (Collector collector, char_t *src, int start, int end)
{
	int size = end - start;
	int index = 0;
	if (size < 0) {
		return;
	}
	while (index < size) {
		if (src[index] == '\0') {
			break;
		}
		collector(index, src[index + start]);
		index++;
	}
	dest[index] = src[index];
}



