#include "inputBuffer.h"
#include <stdlib.h>

inputBuffer* inputBufferCreate(){
    inputBuffer* inputBfr = (inputBuffer*)malloc(sizeof(inputBuffer));
    inputBfr->buffer = NULL;
    inputBfr->bufferLength = 0;
    inputBfr->inputLength = 0;
    return inputBfr;
}

void closeInputBuffer(inputBuffer* inputBfr){
    free(inputBfr->buffer);
    free(inputBfr);
}

void readInput(inputBuffer* inputBfr){
    ssize_t bytesRead = getline(&(inputBfr->buffer), &inputBfr->bufferLength, stdin);

    if (bytesRead <= 0){
        printf("Error reading input\n");
        exit(1);
    }

    inputBfr->inputLength = bytesRead - 1;
    inputBfr->buffer[bytesRead - 1] = 0;
}
