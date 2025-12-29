/* includes */

#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include "../include/Output.h"
#include "../include/KeyProcess.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

/* defines */

#define MAD_INC_EDITOR_VERSION "0.0.1"



enum modes {
    MODE_NORMAL,
    MODE_VISUAL,
    MODE_INSERT,
    MODE_NAVI,
};


/* data */



struct termios org_termios;
/* terminal */

void disableRawMode() {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &org_termios) == -1){
        die("tcsetattr");
    }
}

void editorUpdateRow(erow *row, OutputData *E) {
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

    editorUpdateSyntax(row, E);
}

void editorInsertRow(int at, char *s, size_t len, OutputData *E) {
    if (at < 0 || at > E->numrows) return;
    E->row = realloc(E->row, sizeof(erow) * (E->numrows +1));
    memmove(&E->row[at + 1], &E->row[at], sizeof(erow) * (E->numrows - at));
    for (int j = at + 1; j <= E->numrows; j++) E->row[j].idx++;
    
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

    E->numrows ++;
    E->dirty ++;

}

void enableRawMode() {
    if (tcgetattr(STDIN_FILENO, &org_termios) == -1) {
        die("tcsetattr");
    }
    atexit(disableRawMode);

    struct termios raw = org_termios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_oflag &= ~(CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if ( tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        die("tcsetattr");
    }
}



int getCursorPosiiton(int *rows, int *cols) {
    char buf[32];
    unsigned int i = 0;
    if (write(STDOUT_FILENO, "\x1b[6n", 4 ) != 4) return -1;

    while (i < sizeof (buf) -1 ) {
        if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
        if (buf[i] == 'R') break;
        i++;
    }
    buf[i] = '\0';

    if (buf[0] != '\x1b' || buf[1] != '[') return -1;
    if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return-1;

    printf("\r\n&buf[1]: '%s'\r\n", &buf[1]);

    return 0;
}

int getWinodwSize( int *rows, int *cols) {
    struct winsize ws;

    if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0 ) {
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
        return getCursorPosiiton(rows, cols);
    } else {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}
/* file i/o */


// void filePath(OutputData *E) {
    
//     DIR *pDir;

//     struct dirent *pDirent;


//     pDir = opendir(".");

//     if (pDir == NULL) {
//         die("Cannot Open Directory\\n");
//     }

//     char line[258];  
//     while ((pDirent = readdir(pDir)) != NULL ) {

//         sprintf(line,  "[%s]", pDirent->d_name);
//         // printf(line);

//         editorInsertRow(E->numrows, line, strlen(line), E);
//     }
//     closedir(pDir);

//     E->dirty = 0;
// }

void editorOpen(char *filename, OutputData *E) {
    FILE *fp = fopen(filename, "r");
    E->filename = strdup(filename);

    editorSelectSyntaxHighlight(E);

    if (!fp) die("fopen");

    char *line = NULL;
    size_t linecap = 0;
    ssize_t linelen;
    while ((linelen = getline(&line, &linecap, fp)) != -1 ) {
        while (linelen > 0 && (line[linelen - 1] == '\n' ||
                                line[linelen - 1] == '\r')) linelen--;                        
        editorInsertRow(E->numrows, line, linelen, E);
    }
    free(line);
    fclose(fp);

    E->dirty = 0;
}

/*init editor */
void initEditor(OutputData * E) {
    E->cx = 0;
    E->cy = 0;
    E->rx = 0;
    E->rowoff = 0;
    E->coloff = 0;
    E->numrows = 0;
    E->row = NULL;
    E->filename = NULL;
    E->statusmsg[0] = '\0';
    E->statusmsg_time = 0;
    E->dirty = 0;
    E->syntax = NULL;
    E->mode = MODE_NORMAL;

    if (getWinodwSize(&E->screenRows, &E->screenCols)== -1) die("getWindowSize");
    E->screenRows -= 2;

}

int main (int argc, char *argv[]){
    enableRawMode();    
    OutputData config;
    initEditor(&config);
    if (argc >=2 ) {
        editorOpen( argv[1], &config);
    } else {
        // filePath(&config);
         
    }

    editorSetStatusMessage(&config, "help: ctrl-s = save | ctrl-q = quit | ctrl-f = find");
    while (1){
        editorRefreshScreen(&config);
        editorProcessKeypress(&config);
    }
    return 0;
    
}
