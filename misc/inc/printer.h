#ifndef VM_STD_OUT
#define VM_STD_OUT

#include "ptext.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define GPRINT_WRAP(args ...) \
this->textPosition += snprintf(this->textBuffer + this->textPosition, \
                                this->textSize - this->textPosition, \
                                args);
template <GTEXT_SIZE_T tsize>
class  Printer : public Ptext<tsize> {
        private :
            GTEXT_SIZE_T textPosition;
        public :
            
            Printer ()
            {
                this->textPosition = 0;
                this->clearText();
            }    
                    
            ~Printer ()
            {
                   
            }
            
            void clearText ()
            {
                memset(this->textBuffer, 0, this->textSize);
                this->textPosition = 0;
            }
            
            void setText (char *textBuffer)
            {
                this->clearText();
                GPRINT_WRAP(textBuffer);
            }
            
            void apendText (char *textBuffer)
            {
                GPRINT_WRAP(textBuffer);
            }

            void print (char *str)
            {
                GPRINT_WRAP(str);
            }

            static void print (char *buf, char size, char *str)
            {
                snprintf(buf, size, str);
            }
                
            
            void printLn (char *str)
            {
                GPRINT_WRAP("\n%s", str);
            }
            
            void printInt (int32_t value)
            {
                GPRINT_WRAP("%d", value);
            }

            void printIntLn (int32_t value)
            {
                GPRINT_WRAP("\n%d", value);
            }

            void printUintLn (uint32_t value)
            {
                GPRINT_WRAP("\n%u", value);
            }

            void printHex (int32_t value)
            {
                GPRINT_WRAP("0x%08x", value);
            }

            void printLnHex (int32_t value)
            {
                GPRINT_WRAP("\n0x%08x", value);
            }
            
            void removeLastChar ()
            {
                if (this->textPosition == 0) {
                    return;
                }
                this->textBuffer[--this->textPosition] = '\0';
            }
            
            void insertChar (int position, char c)
            {
                if (this->textPosition == this->textSize)
                    return;
                this->textBuffer[this->textPosition++] = c;
            }
            
            char *getText ()
            {
                return this->textBuffer;
            }
            
            int getTextBufferRemind ()
            {
                return this->textSize - this->textPosition;
            }
            
            int getTextSize ()
            {
                return this->textSize;
            }

    }; /*class printer*/

#endif /*VM_STD_OUT*/


/*End of file*/

