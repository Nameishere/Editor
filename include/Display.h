#ifndef DISPLAY_H
#define DISPLAY_H

#include <time.h>
#include "StateMachine.h"

#define WELCOME_MESSAGE "Welcome to the world of the mad"
#define ABUF_INIT {NULL,0}
#define HL_HIGHLIGHT_NUMBERS (1 << 0)
#define HL_HIGHLIGHT_STRINGS (1 << 1)

enum editorHighlight {
    HL_NORMAL = 0,
    HL_COMMENT,
    HL_MLCOMMENT,
    HL_KEYWORD1,
    HL_KEYWORD2,
    HL_STRING,
    HL_NUMBER,
    HL_MATCH
};


struct editorSyntax {
    char *filetype;
    char **filematch;  
    char **keywords;
    char *singleline_comment_start;  
    char *multiline_comment_start;  
    char *multiline_comment_end;  
    int flags;
};


struct abuf {
    char *b;
    int len;
};


void editorSelectSyntaxHighlight(StateMachine *E);

void editorUpdateSyntax(erow* row, StateMachine *E);

void editorRefreshScreen(StateMachine *E);

#endif
