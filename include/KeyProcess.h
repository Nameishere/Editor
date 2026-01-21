#ifndef KEYPROCESS_H
#define KEYPROCESS_H

#include "../include/StateMachine.h"

typedef struct keyMapping {
    int c;
    int (*callback)(int c, StateMachine *E);
} keyMapping;

typedef struct modeData {
    int (*elseCallback)(int c, StateMachine *E);
    int keyCount;
    keyMapping keys[];
} modeData;


void editorProcessKeypress(StateMachine* E);

#endif


