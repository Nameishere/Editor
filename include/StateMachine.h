/* StateMachine.h */

#ifndef STATEMACHINE_H
#define STATEMACHINE_H


/* 0  */
#include <time.h>


/* 1  */

typedef struct erow {
    int idx;
    int size;
    int rsize;
    char *chars; 
    char *render; 
    unsigned char *hl;
    int hl_open_comment;
} erow;

typedef struct StateMachine {
    int cx;
    int cy;
    int rx;
    int rowOff;
    int colOff;
    int numRows;
    int screenRows;
    int screenCols;
    char* fileName;
    erow *row;
    int dirty;
    int mode;

    char statusMsg[80];
    time_t statusMsg_time;

}StateMachine;


/* 2 */
#define TAB_STOP 8
enum modes {
    MODE_NORMAL = 0,
    MODE_VISUAL,
    MODE_INSERT,
    MODE_NAVI,
};
/* 3 */
void editorOpen(char *fileName, StateMachine *M);
void editorSetStatusMessage(StateMachine *E, const char *fmt, ...);
void editorInsertRow(int at, char *s, size_t len, StateMachine *E);
void editorUpdateRow(erow *row, StateMachine *E);
#endif

