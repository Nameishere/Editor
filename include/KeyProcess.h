#ifndef KEYPROCESS_H
#define KEYPROCESS_H

#include "../include/Output.h"

typedef struct keyMapping {
    int c;
    int (*callback)(int c, OutputData *E);
} keyMapping;


void editorProcessKeypress(OutputData* E);

#endif