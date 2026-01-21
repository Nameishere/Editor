/* StateMachine.c */

/* 0 copyright/licensing */

/* 1 includes */
#include "../include/StateMachine.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 2 defines */

/* 3 external declarations */

/* 4 typedefs */

/* 5 global variable declarations */

/* 6 function prototypes */

void editorSetStatusMessage(StateMachine *E, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(E->statusMsg, sizeof(E->statusMsg), fmt, ap);
  va_end(ap);
  E->statusMsg_time = time(NULL);
}

void editorUpdateRow(erow *row, StateMachine *E) {
    int tabs = 0;
    int j;
    for(j = 0; j < row->size; j++) {
        if (row->chars[j] =='\t') {
            tabs ++;
        }
    }
    free(row->render);
    row->render = malloc(row->size + 1 + tabs * (TAB_STOP - 1));

    int idx = 0;
    for (j=0; j < row->size; j++) {
        if (row->chars[j] == '\t') {
            row->render[idx++] = ' ';
            while (idx % TAB_STOP != 0 ) row->render[idx++] = ' ';
        } else {
            row->render[idx++] = row->chars[j];
        }
    }
    row->render[idx] = '\0';
    row->rsize = idx;

//    editorUpdateSyntax(row, E);
}

void editorInsertRow(int at, char *s, size_t len, StateMachine *E) {
    if (at < 0 || at > E->numRows) return;
    E->row = realloc(E->row, sizeof(erow) * (E->numRows +1)); 
    memmove(&E->row[at + 1], &E->row[at], sizeof(erow) * (E->numRows - at));
    for (int j = at + 1; j <= E->numRows; j++) E->row[j].idx++;
    
    E->row[at].idx = at;

    E->row[at].size = len;
    E->row[at].chars = malloc(len + 1);
    memcpy(E->row[at].chars, s, len);
    E->row[at].chars[len] = '\0';

    E->row[at].rsize = 0;
    E->row[at].render = NULL;
    E->row[at].hl = NULL;
    E->row[at].hl_open_comment = 0;
    editorUpdateRow(&E->row[at], E);

    E->numRows ++;
    E->dirty ++;

}

void editorOpen(char *fileName, StateMachine *M) {
    FILE *fp = fopen(fileName, "r");
    M->fileName = strdup(fileName);

    //editorSelectSyntaxHighlight(E);

    if (!fp) return;

    char *line = NULL;
    size_t linecap = 0;
    ssize_t linelen;
    while ((linelen = getline(&line, &linecap, fp)) != -1 ) {
        while (linelen > 0 && (line[linelen - 1] == '\n' ||
                                line[linelen - 1] == '\r')) linelen--;                        
        editorInsertRow(M->numRows, line, linelen, M);
    }
    free(line);
    fclose(fp);

    M->dirty = 0;
}
    
