#ifndef PASS_PRIMITIVES
#define PASS_PRIMITIVES

#include <stdint.h>


typedef struct {
	const char *op;
	int (*opeation) (int, int, int);
} uInstruction;


typedef struct {
	int (*illegal) (uInstruction *instruction, char *message);
	int (*missingOp) (uInstruction *instruction, char *message);
	int (*incomplete) (uInstruction *instruction, char *message);
	int (*line) (uInstruction *instruction, char *message);
} uHandle;

int uProcess (uHandle handle, const char *input, const char *args);


#endif /*PASS_PRIMITIVES*/

/*End of file*/

