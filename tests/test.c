/* includes */

#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include "../include/Output.h"

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
#define QUIT_TIMES 2

#define CTRL_KEY(k) ((k) & 0x1f)    

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


#define HL_HIGHLIGHT_NUMBERS (1 << 0)
#define HL_HIGHLIGHT_STRINGS (1 << 1)
/* data */


struct termios org_termios;
/* filetypes */

char *C_HL_extensions[] = { ".c", ".h", ".cpp", NULL};
char *C_HL_keywords[] = {
  "switch", "if", "while", "for", "break", "continue", "return", "else",
  "struct", "union", "typedef", "static", "enum", "class", "case",
  "int|", "long|", "double|", "float|", "char|", "unsigned|", "signed|",
  "void|", NULL
};

struct editorSyntax HLDB[] = {
    {
        "c",
        C_HL_extensions,
        C_HL_keywords,
        "//", "/*", "*/",
        HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS
    },
};

#define HLDB_ENTRIES (sizeof(HLDB) / sizeof(HLDB[0]))

/* prototypes */

void editorSetStatusMessage(OutputData *E, const char *fmt, ...);
char *editorPrompt(char *prompt, void (*callback)(char *, int, OutputData *), OutputData *E);

/* terminal */
void die(const char*s) {
    write(STDOUT_FILENO, "\x1b[2j", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
    perror(s);
    exit(1);
}

void disableRawMode() {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &org_termios) == -1){
        die("tcsetattr");
    }
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


int editorReadKey(){
    int nread; 
    char c;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1 ) {
        if (nread == -1 && errno != EAGAIN) die("read");
    }

    if (c == '\x1b') {
        char seq[3];
        if (read(STDIN_FILENO, &seq[0], 1) !=1) return '\x1b';
        if (read(STDIN_FILENO, &seq[1], 1) !=1) return '\x1b';

        if (seq[0] == '[') {
            if (seq[1] >= '0' && seq[1] <= '9') {
                if (read(STDIN_FILENO, &seq[2], 1) !=1) return '\x1b';
                if (seq[2] == '~') {
                    switch (seq[1]) {
                        case '1': return HOME_KEY;
                        case '3': return DEL_KEY;
                        case '4': return END_KEY;
                        case '5': return PAGE_UP;
                        case '6': return PAGE_DOWN;
                        case '7': return HOME_KEY;
                        case '8': return END_KEY;
                    }
                } 
                
            } else {
                switch (seq[1]) {
                    case 'A': return ARROW_UP;
                    case 'B': return ARROW_DOWN;
                    case 'C': return ARROW_RIGHT;
                    case 'D': return ARROW_LEFT;
                    case 'H': return HOME_KEY;
                    case 'F': return END_KEY;
                }
            }
        } else if (seq[0] == 'O') {
            switch (seq[1]) {
                case 'H': return HOME_KEY;
                case 'F': return END_KEY;
            }
        }

        return '\x1b';
    } else {
        return c;
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


/* Syntax highlighting */

int is_separator(int c) {
    return isspace(c) || c == '\0' || strchr(",.()+-/*=~%<>[];", c) != NULL;
}
void editorUpdateSyntax(erow *row, OutputData *E) {
    row->hl = realloc(row->hl, row->rsize);
    memset(row->hl, HL_NORMAL, row->rsize);

    if (E->syntax == NULL) return;

    char **keywords = E->syntax->keywords;

    char *scs = E->syntax->singleline_comment_start;
    char *mcs = E->syntax->multiline_comment_start;
    char *mce = E->syntax->multiline_comment_end;

    int scs_len = scs ? strlen(scs) : 0;
    int mcs_len = mcs ? strlen(mcs) : 0;
    int mce_len = mce ? strlen(mce) : 0;

    int prev_sep = 1; 
    int in_string = 0;
    int in_comment= (row->idx > 0 && E->row[row->idx - 1].hl_open_comment);

    int i = 0;
    while (i < row->rsize) {
        char c = row->render[i];
        unsigned char prev_hl = (i > 0) ? row->hl[i - 1] : HL_NORMAL;

        if (scs_len && !in_string && !in_comment) {
            if (!strncmp(&row->render[i], scs, scs_len)) {
                memset(&row->hl[i], HL_COMMENT, row->rsize - i);
                break;
            }
        }


        if (mcs_len && mce_len && !in_string) {
            if (in_comment) {
                row->hl[i] = HL_MLCOMMENT;
                if (!strncmp(&row->render[i], mce, mce_len)) {
                    memset(&row->hl[i], HL_MLCOMMENT, mce_len);
                    i += mce_len;
                    in_comment = 0;
                    prev_sep = 1;
                    continue;
                } else {
                    i++;
                    continue;
                }
            } else if (!strncmp(&row->render[i], mcs,mcs_len)) {    
                memset(&row->hl[i], HL_MLCOMMENT, mcs_len);
                i += mcs_len;
                in_comment = 1;
                continue;
            }
        }
        
        if (E->syntax->flags & HL_HIGHLIGHT_STRINGS) {
            if (in_string) {
                row->hl[i] = HL_STRING;
                if (c == '\\' && i + 1 < row->rsize) {
                    row->hl[i + 1] = HL_STRING;
                    i += 2;
                    continue;

                }
                if (c == in_string) in_string = 0;
                i++;
                prev_sep = 1;
                continue;

            } else {
                if (c == '"' || c == '\'') {
                    in_string = c;
                    row->hl[i] = HL_STRING;
                    i++;
                    continue;
                }
            }
        }

        if (E->syntax->flags & HL_HIGHLIGHT_NUMBERS) {
            if ((isdigit(c) && (prev_sep || prev_hl == HL_NUMBER)) || (c == '.' && prev_hl == HL_NUMBER)) {
                row->hl[i] = HL_NUMBER;
                i++;
                prev_sep = 0;
                continue;

            }
        }

        if (prev_sep) {
            int j;
            for (j = 0; keywords[j]; j++) {
                int klen = strlen(keywords[j]);
                int kw2 = keywords[j][klen - 1] == '|';
                if (kw2) klen --;

                if (!strncmp(&row->render[i], keywords[j], klen) && is_separator(row->render[i + klen])) {
                    memset (&row->hl[i], kw2 ? HL_KEYWORD2 : HL_KEYWORD1, klen);
                    i += klen;
                    break;
                }
            }
            if (keywords[j] != NULL) {
                prev_sep = 0;
                continue;
            } 
        }

        prev_sep = is_separator(c);
        i++;
    }

    int changed = (row->hl_open_comment != in_comment);
    row->hl_open_comment = in_comment;
    if (changed && row->idx + 1 < E->numrows)
        editorUpdateSyntax(&E->row[row->idx + 1],  E);
}


void editorSelectSyntaxHighlight(OutputData *E) {
    E->syntax = NULL;
    if (E->filename == NULL) return;

    char *ext = strrchr(E->filename, '.');

    for (unsigned int j = 0; j < HLDB_ENTRIES; j++) {
        struct editorSyntax *s = &HLDB[j];
        unsigned int i = 0;
        while (s->filematch[i]) {
            int is_ext = (s->filematch[i][0] == '.');
            if ((is_ext && ext && !strcmp(ext, s->filematch[i])) || (!is_ext && strstr(E->filename, s->filematch[i]))) {
                E->syntax = s;

                int filerow;
                for (filerow = 0; filerow < E->numrows; filerow++) {
                    editorUpdateSyntax(&E->row[filerow], E);
                }

                return;
            }
            i++;
        }
    }
}

/* row operations */


int editorRowRxToCx(erow *row, int rx) {
    int cur_rx = 0;
    int cx;
    for (cx = 0; cx < row->size; cx++) {
        if (row ->chars[cx] == '\t')
            cur_rx += (TAB_STOP - 1) - (cur_rx % TAB_STOP);
        cur_rx ++;

        if (cur_rx > rx) return cx;
    }
    return cx;
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

void editorFreeRow(erow *row) {
    free(row->render);
    free(row->chars);
    free(row->hl);
}

void editorDelRow(int at, OutputData *E) {
    if (at < 0 || at >= E->numrows) return;
    editorFreeRow(&E->row[at]);
    memmove(&E->row[at], &E->row[at + 1], sizeof(erow) * (E->numrows - at -1));
    for (int j = at; j < E->numrows - 1; j++) E->row[j].idx--;
    E->numrows--;
    E->dirty++;
}

void editorRowInsertChar(erow *row, int at, int c, OutputData *E) {
    if (at < 0 || at > row->size) at = row->size;
    row->chars = realloc(row->chars, row->size + 2);
    memmove(&row->chars[at + 1], &row->chars[at], row->size - at + 1);
    row->size++;
    row->chars[at] = c;
    editorUpdateRow(row, E);
    E->dirty ++;
}

void editorRowAppendString(erow *row, char *s, size_t len, OutputData *E) {
    row->chars = realloc(row->chars, row->size + len + 1);
    memcpy(&row->chars[row->size], s, len);
    row->size += len;
    row->chars[row->size] = '\0';
    editorUpdateRow(row, E);
    E->dirty++;
}

void editorRowDelChar(erow * row, int at, OutputData *E) {  
    if (at < 0 || at > row->size) return;
    memmove(&row->chars[at], &row->chars[at + 1], row->size - at);
    row->size--;
    editorUpdateRow(row, E);
    E->dirty ++;
}

void editorInsertChar(int c, OutputData *E) {
    if (E->cy == E->numrows) { 
        editorInsertRow(E->numrows,"", 0, E);
    }
    editorRowInsertChar(&E->row[E->cy], E->cx, c, E);

    E->cx++;
}

void editorInsertNewLine(OutputData *E) {
    if (E->cx == 0 ) {
        editorInsertRow(E->cy, "", 0, E);
    } else {
        erow *row = &E->row[E->cy];
        editorInsertRow(E->cy + 1, &row->chars[E->cx], row->size - E->cx, E);
        row = &E->row[E->cy];
        row->size = E->cx;
        row->chars[row->size] = '\0';
        editorUpdateRow(row, E);
    }
    E->cy ++;
    E->cx = 0;
}

void editorSetStatusMessage(OutputData *E, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(E->statusmsg, sizeof(E->statusmsg), fmt, ap);
  va_end(ap);
  E->statusmsg_time = time(NULL);
}


void editorDelChar(OutputData *E ) {
    if (E->cy == E->numrows) { 
        return;
    }
    if (E->cx ==0 && E->cy ==0) return;

    erow *row = &E->row[E->cy];
    if (E->cx > 0 ) {
        editorRowDelChar(row, E->cx -1, E);
        E->cx --;
    } else {
        E->cx = E->row[E->cy - 1].size;
        editorRowAppendString(&E->row[E->cy - 1], row->chars, row->size, E);
        editorDelRow(E->cy, E);
        E->cy--;

    }
}

/* file i/o */

char *editorRowsToString(int *buflen, OutputData *E) {
    int totlen = 0;
    int j;
    for (j = 0; j < E->numrows; j++) {
        totlen += E->row[j].size + 1;
    }

    *buflen = totlen;

    char* buf = malloc(totlen);
    char*p = buf;
    for (j = 0; j < E->numrows; j++) {
        memcpy(p, E->row[j].chars, E->row[j].size);
        p += E->row[j].size;
        *p = '\n';
        p++;
    }

    return buf;
}

void filePath(OutputData *E) {
    
    DIR *pDir;

    struct dirent *pDirent;


    pDir = opendir(".");

    if (pDir == NULL) {
        die("Cannot Open Directory\\n");
    }

    char line[258];  
    while ((pDirent = readdir(pDir)) != NULL ) {

        sprintf(line,  "[%s]", pDirent->d_name);
        // printf(line);

        editorInsertRow(E->numrows, line, strlen(line), E);
    }
    closedir(pDir);

    E->dirty = 0;
}

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

void editorSave(OutputData *E) {
    if ( E->filename == NULL)  {
        E->filename = editorPrompt("Save as: %s (ESC to cancel)", NULL, E);
        if (E->filename == NULL) {
            editorSetStatusMessage(E, "Save aborted");
            return;
        }
        editorSelectSyntaxHighlight(E);
    }

    int len; 
    char *buf = editorRowsToString (&len, E);

    int fd = open(E->filename, O_RDWR | O_CREAT, 0644);
    if (fd != -1) {
        if (ftruncate(fd, len) != -1) {
            if (write(fd,buf, len) == len) {
                close(fd);
                free(buf);
                E->dirty = 0;
                char str[33];
                sprintf(str,"%d bytes witten to disk", len);
                editorSetStatusMessage(E, str);
                return;
            }
        }
        close(fd);
    }

    free(buf);
    char str[24];
    sprintf(str, "Can't save! I/O error: %s", strerror(errno));
    editorSetStatusMessage(E, str);
}

/* find */

void editorFindCallback(char * query, int key, OutputData *E) {
    static int last_match = -1;
    static int direction = 1;

    static int saved_hl_line;
    static char *saved_hl = NULL;

    if (saved_hl) {
        memcpy(E->row[saved_hl_line].hl, saved_hl, E->row[saved_hl_line].rsize);
        free(saved_hl);
        saved_hl = NULL;
    }

    if (key == '\r' || key == '\x1b') {
        last_match = -1;
        direction = 1;
        return;
    } else if (key == ARROW_RIGHT || key == ARROW_DOWN) {
        direction = 1 ;
    } else if (key == ARROW_LEFT || key == ARROW_UP){
        direction = -1;
    } else {
        last_match = -1;
        direction = 1;
    }

    if (last_match == -1) direction = 1;
    int current = last_match;
    int i; 
    for (i = 0; i < E->numrows; i++) {
        current += direction;
        if (current == -1) current = E->numrows - 1;
        else if (current == E->numrows) current = 0;

        erow * row = &E->row[current];
        char * match = strstr(row->render, query);
        if (match) {
            last_match = current;
            E->cy = current;
            E->cx = editorRowRxToCx(row,match- row->render); 
            E->rowoff =E->numrows;

            saved_hl_line = current;
            saved_hl = malloc(row->rsize);
            memcpy(saved_hl, row->hl, row->rsize);
            memset(&row->hl[match - row->render], HL_MATCH, strlen(query));
            break;
        }
    }
}

void editorFind( OutputData * E) {
    int saved_cx = E->cx;
    int saved_cy = E->cy;
    int saved_coloff = E->coloff;
    int savedcol_rowoff = E->rowoff;

    char * query = editorPrompt("Search: %s (ESC/Arrows/Enter)", editorFindCallback, E);
    if (query) {
        free(query);
    } else {
    E->cx =      saved_cx;
    E->cy =      saved_cy;
    E->coloff =  saved_coloff;
    E->rowoff =  savedcol_rowoff;
    }
}
/* append buffer */



/* input */

char *editorPrompt(char *prompt, void (*callback)(char *, int, OutputData *), OutputData * E) {
    size_t bufsize = 128;
    char *buf = malloc(bufsize);

    size_t buflen =0;
    buf[0] = '\0';

    while (1) {
        editorSetStatusMessage(E, prompt, buf);
        editorRefreshScreen(E);

        int c = editorReadKey();
        if (c == DEL_KEY || c == CTRL_KEY('h') || c == BACKSPACE) {
            if (buflen != 0 ) buf[--buflen] = '\0';
        } else if (c== '\x1b') {
            editorSetStatusMessage(E, "");
            if (callback) callback(buf, c, E);
            free(buf);
            return NULL;
        } else if (c == '\r') {
            if (buflen != 0) {
                editorSetStatusMessage(E, "");
                if (callback) callback(buf, c, E);
                return buf;
            }
        } else if (!iscntrl(c) && c < 128){
            if (buflen == bufsize -1) {
                bufsize *= 2;
                buf = realloc(buf, bufsize);
            }
            buf[buflen++] = c;
            buf[buflen] = '\0';
        }

        if (callback) callback(buf, c, E);
    }
}


void editorMoveCursor(int key, OutputData *E) {
    erow *row = E->cy >= E->numrows ? NULL : &E->row[E->cy];
    switch (key) {
        case ARROW_LEFT:
            if ( E->cx != 0) {
                E->cx--;
            } else if (E->cy > 0 ) {
                E->cy --;
                E->cx = E->row[E->cy].size;
            }
            break;
        case ARROW_DOWN:
            if (E->cy < E->numrows) {
                E->cy++;
            }else if (E->rowoff < E->numrows){
                E->rowoff ++;
            }
            break;
        case ARROW_UP:
            if ( E->cy != 0) {
                E->cy--;
            }
            break;
        case ARROW_RIGHT:
            if (row && E->cx < row->size) {
                E->cx++;
            } else if (row && E->cx == row->size) {
                E->cy++;
                E->cx = 0;
            }
            break;
    }

    row = E->cy >= E->numrows ? NULL : & E->row[E->cy];
    int rowlen = row ? row->size : 0;
    if (E->cx > rowlen) {
        E->cx = rowlen;
    }
}

void editorProcessKeypress(OutputData* E){
    int c = editorReadKey();
    static int quit_times = QUIT_TIMES;

    switch(c) {
        case '\r':
            editorInsertNewLine(E);
            break;
        case CTRL_KEY('q'):
            if (E->dirty && quit_times > 0) {
                editorSetStatusMessage(E, "WARNING !!! File has unsaved changes. "
                    "Press Ctrl-Q %d more times to quit", quit_times);
                quit_times--;
                return;
            }
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(0);
            break;
        case CTRL_KEY('s'):
            editorSave(E);
            break;
        case HOME_KEY:
            E->cx = 0;
            break;
        case END_KEY:
            if (E->cy < E->numrows) {
                E->cx = E->row[E->cy].size;
            }
            break;        
        case CTRL_KEY('F'):
            editorFind(E);
            break;    
        case BACKSPACE:
        case CTRL_KEY('h'):
        case DEL_KEY:
            if (c == DEL_KEY) editorMoveCursor(ARROW_RIGHT, E);
            editorDelChar(E);
            break;
        case PAGE_UP:
        case PAGE_DOWN:
            {
                if (c == PAGE_UP) {
                    E->cy = E->rowoff;
                } else if (c == PAGE_DOWN) {
                    E->cy = E->rowoff + E->screenRows - 1;
                    if (E->cy > E->numrows) E->cy = E->numrows;
                }

                int times = E->screenRows;
                while (times --) editorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN, E);
            }
            break;        
        case ARROW_LEFT:
        case ARROW_RIGHT:
        case ARROW_UP:
        case ARROW_DOWN:
            editorMoveCursor(c, E);
            break;
        case CTRL_KEY('l'):
        case '\x1b':
            break;
        default:
            editorInsertChar(c, E);
            break;
    }
    quit_times = QUIT_TIMES;
}

/* init */

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
        filePath(&config);
         
    }

    editorSetStatusMessage(&config, "help: ctrl-s = save | ctrl-q = quit | ctrl-f = find");
    while (1){
        editorRefreshScreen(&config);
        editorProcessKeypress(&config);
    }
    return 0;
    
}
