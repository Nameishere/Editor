#ifndef OUTPUT_H
#define OUTPUT_H

#include <time.h>

#define TAB_STOP 8
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

typedef struct erow {
    int idx;
    int size;
    int rsize;
    char *chars; 
    char *render; 
    unsigned char *hl;
    int hl_open_comment;
} erow;

struct editorSyntax {
    char *filetype;
    char **filematch;  
    char **keywords;
    char *singleline_comment_start;  
    char *multiline_comment_start;  
    char *multiline_comment_end;  
    int flags;
};

typedef struct OutputData {
    int mode;

    int rx;
    int cx;
    int cy;

    int numrows;
    int rowoff;
    int screenRows;

    int coloff;
    int screenCols;

    char * filename;
    erow *row;

    int dirty;

    struct editorSyntax *syntax;
    char statusmsg[80];
    time_t statusmsg_time;


} OutputData;


struct abuf {
    char *b;
    int len;
};


void editorSelectSyntaxHighlight(OutputData *E);

void editorUpdateSyntax(erow* row, OutputData *E);

void editorSetStatusMessage(OutputData *E, const char *fmt, ...);

void editorRefreshScreen(OutputData *E);

void die(const char*s);

#endif