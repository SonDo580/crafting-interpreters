#ifndef clox_line_h
#define clox_line_h

typedef struct
{
    int capacity;
    int count;
    int *lines_rle; // 1 item = 2 slots (for line & count)
} LineArray;

void initLineArray(LineArray *array);
void writeLineArray(LineArray *array, int line);
void freeLineArray(LineArray *array);
int getLine(LineArray *array, int offset);

#endif