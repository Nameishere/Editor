#ifndef KEYPRESSFUNCTIONS_H
#define KEYPRESSFUNCTIONS_H

#include "../include/StateMachine.h"
#define CTRL_KEY(k) ((k) & 0x1f)    

#define PUNCTUATION "!@#$%^&*()[]{}>?/.,<'\""
#define STATUS_SIZE 2

#define QUIT_TIMES 2
enum editorKey {
    ESC_KEY = 27,
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
int editorMoveCursorUp(int key, StateMachine *E);
int editorMoveCursorDown(int key, StateMachine *E);
int editorMoveCursorLeft(int key, StateMachine *E);
int editorMoveCursorRight(int key, StateMachine *E);
int editorQuitApp(int key, StateMachine *E);
int editorToNormalMode(int key, StateMachine *E);
int editorToInsertMode(int key, StateMachine *E);
int editorMoveCursorScreenTop(int key, StateMachine *E);
int editorMoveCursorScreenMiddle(int key, StateMachine *E);
int editorMoveCursorScreenBottom(int key, StateMachine *E);
int editorMoveCursorWordStartNP(int key, StateMachine *E);
int editorMoveCursorWordStart(int key, StateMachine *E);
int editorMoveCursorWordEndNP(int key, StateMachine *E);
int editorMoveCursorWordEnd(int key, StateMachine *E);
int editorMoveCursorLastWordStartNP(int key, StateMachine *E);
int editorMoveCursorLastWordStart(int key, StateMachine *E);
int doNothing(int key, StateMachine *E);
int editorInsertNewLine(int key, StateMachine *E);
int editorInsertChar(int key, StateMachine *E);

#endif
