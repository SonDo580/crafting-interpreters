#ifndef clox_line_h
#define clox_line_h

// Run-length encoding: line number - number of instructions on that line
typedef struct
{
    int count;
    int capacity;
    int *entries;
} LineArray;

void initLineArray(LineArray *array);
void writeLineArray(LineArray *array, int lineNumber);
void freeLineArray(LineArray *array);
int getLine(LineArray *array, int instructionIndex);

#endif