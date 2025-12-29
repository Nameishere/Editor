#ifndef KEYPRESSFUNCTIONS_H
#define KEYPRESSFUNCTIONS_H

#include "../include/Output.h"
#define CTRL_KEY(k) ((k) & 0x1f)    

#define QUIT_TIMES 2
enum editorKey {
    BACKSPACE   = 127,
    ARROW_LEFT   =1000,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
    DEL_KEY,
    PAGE_UP,
    PAGE_DOWN,
    HOME_KEY,
    END_KEY,
};


int editorReadKey();


int editorMoveCursorUp(int key, OutputData *E);
int editorMoveCursorDown(int key, OutputData *E);
int editorMoveCursorLeft(int key, OutputData *E);
int editorMoveCursorRight(int key, OutputData *E);
int editorQuitApp(int key, OutputData *E);

#endif