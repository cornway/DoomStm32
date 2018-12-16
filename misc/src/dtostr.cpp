#include "dtostr.h"
#include "string.h"

						char *reverseChars (char *charSequence)
            {
                  char *j;
                  int32_t c;
                 
                  j = charSequence + strlen(charSequence) - 1;
                  while(charSequence < j) {
                    c = *charSequence;
                    *charSequence++ = *j;
                    *j-- = c;
                  }	
                  return j;	
            }    
            char *stringifyInt (char *buffer, int32_t value, int32_t base)
            {
                  int32_t i, sign;
                  if ((sign = value) < 0)              /* record sign */
                      value = -value;                    /* make n positive */
                  i = 0;
                  do {                               /* generate digits in reverse order */
                      buffer[i++] = value % base + '0';   /* get next digit */
                  } while ((value /= base) > 0);       /* delete it */
                  if (sign < 0)
                      buffer[i++] = '-';
                  //buffer[i] = '\0';
                  return reverseChars(buffer);
            }
            char *stringifyInt (char *buffer, uint32_t value, int32_t base)
            {
                  int32_t i;                   /* make n positive */
                  i = 0;
                  do {                               /* generate digits in reverse order */
                      buffer[i++] = value % base + '0';   /* get next digit */
                  } while ((value /= base) > 0);       /* delete it */
                  //buffer[i] = '\0';
                  return reverseChars(buffer);
            }
						
						
						
