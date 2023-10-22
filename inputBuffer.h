#pragma once
#include <stddef.h>
#include <stdio.h>

typedef struct inputBuffer{
    char* buffer;
    size_t bufferLength;
    ssize_t inputLength;
} inputBuffer;

inputBuffer* inputBufferCreate();

void closeInputBuffer(inputBuffer* inputBfr);

void readInput(inputBuffer* inputBfr);
