#ifndef XML_PARSER 
#define XML_PARSER

#include <stdint.h>

__weak void * operator new (uint32_t size);


namespace xml {
	
template <class NodeList, class Document, class File>
	class DocumentBuilder {
		private :
			char *lineContainer;
		public :
			DocumentBuilder () {}
			Document *getDoc (File *file)
			{
					Document *document;
				  NodeList headList;
				  char *sequence = (char *)0;
					if (file == (File *)0) {
						throw -1;
					}
					for (;;) {
						sequence = file->readUnbufLine(/*read attributes*/);
						if (sequence == (char *)0) {
							throw 1;
						}
						unsigned int charCount = file->lastLineLength();
						char *memory = new char[charCount + 1];
						int index = 0;
						while (index < charCount) {
							memory[index] = sequence[index];
							index++;
						}
						lineContainer = memory;
						lineContainer[index] = '/n';
						document->appendLine(lineContainer);
					}
					return 0;
			}
			
	};
};


#endif /*XML_PARSER*/


/*end of file*/

