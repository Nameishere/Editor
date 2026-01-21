// This file contains all the functions that can be actibated by keypresses or by running 
// them in the command line 

// callback function format = int func(int c, StateMachine E);
#include "../include/KeyPressFunctions.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

char *editorRowsToString(int *buflen, StateMachine *E) {
    int totlen = 0;
    int j;
    for (j = 0; j < E->numRows; j++) {
        totlen += E->row[j].size + 1;
    }

    *buflen = totlen;

    char* buf = malloc(totlen);
    char*p = buf;
    for (j = 0; j < E->numRows; j++) {
        memcpy(p, E->row[j].chars, E->row[j].size);
        p += E->row[j].size;
        *p = '\n';
        p++;
    }
    return buf;
}

int editorReadKey(){
    int nread; 
    char c;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1 ) {
        //if (nread == -1 && errno != EAGAIN) die("read");
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

        return ESC_KEY;
    } else {
        return c;
    }
}
void FixCursorPosition(StateMachine *E) {
    erow *row = E->cy >= E->numRows ? NULL : & E->row[E->cy];
    int rowlen = row ? row->size : 0;
    if (E->cx > rowlen) {
        E->cx = rowlen;
    }
}


int editorMoveCursorUp(int key, StateMachine *E) {
    if ( E->cy != 0) {
        E->cy--;
    }
    FixCursorPosition(E);
    return 1;
}

int editorMoveCursorDown(int key, StateMachine *E) {
    if (E->cy < E->numRows) {
        E->cy++;
    }else if (E->rowOff < E->numRows){
        E->rowOff ++;
    }
    FixCursorPosition(E);
    return 1;
}

int editorMoveCursorLeft(int key, StateMachine *E) {
    if ( E->cx != 0) {
        E->cx--;
    } else if (E->cy > 0 ) {
        E->cy --;
        E->cx = E->row[E->cy].size;
    }
    return 1;
}

int editorMoveCursorRight(int key, StateMachine *E) {
    erow *row = E->cy >= E->numRows ? NULL : &E->row[E->cy];
    if (row && E->cx < row->size) {
        E->cx++;
    } else if (row && E->cx == row->size) {
        E->cy++;
        E->cx = 0;
    }
    FixCursorPosition(E);
    return 1;
}

int editorQuitApp(int key, StateMachine *E) {
    static int quit_times = QUIT_TIMES;
    if (E->dirty && quit_times > 0) {
        editorSetStatusMessage(E, "WARNING !!! File has unsaved changes. "
            "Press Ctrl-Q %d more times to quit", quit_times);
        quit_times--;
        return 1;
    }
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
    exit(0);
    return 1;
}

int editorToInsertMode(int key, StateMachine *E) {
    E->mode = MODE_INSERT;
}

int editorToNormalMode(int key, StateMachine *E) {
    E->mode = MODE_NORMAL;
}

int editorMoveCursorScreenTop(int key, StateMachine *E) {
    E->cy = E->rowOff;
    FixCursorPosition(E);
}

int editorMoveCursorScreenMiddle(int key, StateMachine *E) {
    E->cy = E->rowOff + (E->screenRows - STATUS_SIZE + 1)/2;
    FixCursorPosition(E);
}

int editorMoveCursorScreenBottom(int key, StateMachine *E) {
    E->cy = E->rowOff + E->screenRows - STATUS_SIZE +  1;
    FixCursorPosition(E);
}

int editorMoveCursorWordStartNP(int key, StateMachine *E) {
    char current;
    bool foundSpace = false;
    while (1) {
        current = E->row[E->cy].chars[E->cx]; 
        if ( current == ' ' || E->row[E->cy].size == E->cx) {
            editorMoveCursorRight(32, E);
            foundSpace = true;
        } else if (foundSpace && current != ' ') {
            return 1;
        } else {
            editorMoveCursorRight(32, E);
        }
    }
}

int editorMoveCursorWordStart(int key, StateMachine *E) {
    char lastChar = E->row[E->cy].chars[E->cx];
    bool lastCharIsPunc = strchr(PUNCTUATION, lastChar) != NULL; 
    bool newLine = E->row[E->cy].size == E->cx;
    editorMoveCursorRight(32, E);
    char currentChar = E->row[E->cy].chars[E->cx]; 
    bool currentCharIsPunc = strchr(PUNCTUATION, currentChar) != NULL; 
    while (1) {
        if ((lastChar == ' ' || newLine) && currentChar != ' '){
            return 1;
        } else if (lastCharIsPunc && !currentCharIsPunc && currentChar != ' ') {
            return 1;
        } else if (!lastCharIsPunc && currentCharIsPunc && currentChar != ' ' && currentChar != '\0') {
            return 1;
        } else {
            lastChar = currentChar;
            lastCharIsPunc = currentCharIsPunc; 
            newLine = E->row[E->cy].rsize == E->cx;
            editorMoveCursorRight(32, E);
            currentChar = E->row[E->cy].chars[E->cx]; 
            currentCharIsPunc = strchr(PUNCTUATION, currentChar) != NULL; 
        }

    }
}

int editorMoveCursorWordEndNP(int key, StateMachine *E) {
    char nextChar;
    char currentChar;
    bool newLine;
    editorMoveCursorRight(32, E);
    while (1) {
        currentChar = E->row[E->cy].chars[E->cx];
        if (E->cx < E->row[E->cy].size) {
            nextChar = E->row[E->cy].chars[E->cx + 1];
        } else if (E->cx == E->row[E->cy].size) {
            nextChar = E->row[E->cy + 1].chars[0];
        }

        if ((nextChar== ' ' || nextChar== '\0') && currentChar != ' ' && currentChar != '\0') {
            return 1;
        } else {
            editorMoveCursorRight(32, E);
        }
    }
}

int editorMoveCursorWordEnd(int key, StateMachine *E) {
    char nextChar;
    char currentChar;
    bool nextCharIsPunc;
    bool currentCharIsPunc;
    bool newLine;
    editorMoveCursorRight(32, E);
    while (1) {
        currentChar = E->row[E->cy].chars[E->cx];
        currentCharIsPunc = strchr(PUNCTUATION, currentChar) != NULL;
        if (E->cx < E->row[E->cy].size) {
            nextChar = E->row[E->cy].chars[E->cx + 1];
        } else if (E->cx == E->row[E->cy].size) {
            nextChar = E->row[E->cy + 1].chars[0];
        }
        nextCharIsPunc = strchr(PUNCTUATION, nextChar) != NULL;

        if ((nextChar== ' ' || nextChar== '\0') && currentChar != ' ' && currentChar != '\0') {
            return 1;
        } else if (nextCharIsPunc && !currentCharIsPunc && currentChar != ' ') {
            return 1;
        } else if (!nextCharIsPunc && currentCharIsPunc && currentChar != ' ' && currentChar != '\0') {
            return 1;
        } else {
            editorMoveCursorRight(32, E);
        }
    }
}

int editorMoveCursorLastWordStartNP(int key, StateMachine *E) {
    char nextChar;
    char currentChar;
    bool newLine;
    editorMoveCursorLeft(32, E);
    while (1) {
        currentChar = E->row[E->cy].chars[E->cx];
        if (E->cx > 0) {
            nextChar = E->row[E->cy].chars[E->cx - 1];
        } else if (E->cx == 0) {
            nextChar = E->row[E->cy - 1].chars[E->row[E->cy - 1].size];
        }

        if ((nextChar== ' ' || nextChar== '\0') && currentChar != ' ' && currentChar != '\0') {
            return 1;
        } else {
            editorMoveCursorLeft(32, E);
        }
    }
}

int editorMoveCursorLastWordStart(int key, StateMachine *E) {
    char nextChar;
    char currentChar;
    bool nextCharIsPunc;
    bool currentCharIsPunc;
    bool newLine;
    editorMoveCursorLeft(32, E);
    while (1) {
        currentChar = E->row[E->cy].chars[E->cx];
        currentCharIsPunc = strchr(PUNCTUATION, currentChar) != NULL;
        if (E->cx > 0) {
            nextChar = E->row[E->cy].chars[E->cx - 1];
        } else if (E->cx == 0) {
            nextChar = E->row[E->cy - 1].chars[E->row[E->cy - 1].size];
        }
        nextCharIsPunc = strchr(PUNCTUATION, nextChar) != NULL;

        if ((nextChar== ' ' || nextChar== '\0') && currentChar != ' ' && currentChar != '\0') {
            return 1;
        } else if (nextCharIsPunc && !currentCharIsPunc && currentChar != ' ') {
            return 1;
        } else if (!nextCharIsPunc && currentCharIsPunc && currentChar != ' ' && currentChar != '\0') {
            return 1;
        } else {
            editorMoveCursorLeft(32, E);
        }
    }
}

int doNothing(int key, StateMachine *E) {
    return 1;
}
//char *editorPrompt(char *prompt, void (*callback)(char *, int, StateMachine *), StateMachine * E) {
//     size_t bufsize = 128;
//     char *buf = malloc(bufsize);
//
//     size_t buflen =0;
//     buf[0] = '\0';
//
//     while (1) {
//         editorSetStatusMessage(E, prompt, buf);
//         editorRefreshScreen(E);
//
//         int c = editorReadKey();
//         if (c == DEL_KEY || c == CTRL_KEY('h') || c == BACKSPACE) {
//             if (buflen != 0 ) buf[--buflen] = '\0';
//         } else if (c== '\x1b') {
//             editorSetStatusMessage(E, "");
//             if (callback) callback(buf, c, E);
//             free(buf);
//             return NULL;
//         } else if (c == '\r') {
//             if (buflen != 0) {
//                 editorSetStatusMessage(E, "");
//                 if (callback) callback(buf, c, E);
//                 return buf;
//             }
//         } else if (!iscntrl(c) && c < 128){
//             if (buflen == bufsize -1) {
//                 bufsize *= 2;
//                 buf = realloc(buf, bufsize);
//             }
//             buf[buflen++] = c;
//             buf[buflen] = '\0';
//         }
//
//         if (callback) callback(buf, c, E);
//     }
//}

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

//void editorFindCallback(char * query, int key, StateMachine *E) {
//     static int last_match = -1;
//     static int direction = 1;
//
//     static int saved_hl_line;
//     static char *saved_hl = NULL;
//
//     if (saved_hl) {
//         memcpy(E->row[saved_hl_line].hl, saved_hl, E->row[saved_hl_line].rsize);
//         free(saved_hl);
//         saved_hl = NULL;
//     }
//
//     if (key == '\r' || key == '\x1b') {
//         last_match = -1;
//         direction = 1;
//         return;
//     } else if (key == ARROW_RIGHT || key == ARROW_DOWN) {
//         direction = 1 ;
//     } else if (key == ARROW_LEFT || key == ARROW_UP){
//         direction = -1;
//     } else {
//         last_match = -1;
//         direction = 1;
//     }
//
//     if (last_match == -1) direction = 1;
//     int current = last_match;
//     int i; 
//     for (i = 0; i < E->numRows; i++) {
//         current += direction;
//         if (current == -1) current = E->numRows - 1;
//         else if (current == E->numRows) current = 0;
//
//         erow * row = &E->row[current];
//         char * match = strstr(row->render, query);
//         if (match) {
//             last_match = current;
//             E->cy = current;
//             E->cx = editorRowRxToCx(row,match- row->render); 
//             E->rowOff =E->numRows;
//
//             saved_hl_line = current;
//             saved_hl = malloc(row->rsize);
//             memcpy(saved_hl, row->hl, row->rsize);
//             memset(&row->hl[match - row->render], HL_MATCH, strlen(query));
//             break;
//         }
//     }
//}

//int editorFind(int key, StateMachine * E) {
//     int saved_cx = E->cx;
//     int saved_cy = E->cy;
//     int saved_coloff = E->coloff;
//     int savedcol_rowoff = E->rowOff;
//
//     char * query = editorPrompt("Search: %s (ESC/Arrows/Enter)", editorFindCallback, E);
//     if (query) {
//         free(query);
//     } else {
//     E->cx =      saved_cx;
//     E->cy =      saved_cy;
//     E->coloff =  saved_coloff;
//     E->rowOff =  savedcol_rowoff;
//     }
//     return 1;
//}






//void editorSave(StateMachine *E) {
//     if ( E->filename == NULL)  {
//         E->filename = editorPrompt("Save as: %s (ESC to cancel)", NULL, E);
//         if (E->filename == NULL) {
//             editorSetStatusMessage(E, "Save aborted");
//             return;
//         }
//         editorSelectSyntaxHighlight(E);
//     }
//
//     int len; 
//     char *buf = editorRowsToString (&len, E);
//
//     int fd = open(E->filename, O_RDWR | O_CREAT, 0644);
//     if (fd != -1) {
//         if (ftruncate(fd, len) != -1) {
//             if (write(fd,buf, len) == len) {
//                 close(fd);
//                 free(buf);
//                 E->dirty = 0;
//                 char str[33];
//                 sprintf(str,"%d bytes witten to disk", len);
//                 editorSetStatusMessage(E, str);
//                 return;
//             }
//         }
//         close(fd);
//     }
//
//     free(buf);
//     char str[24];
//     sprintf(str, "Can't save! I/O error: %s", strerror(errno));
//     editorSetStatusMessage(E, str);
//}

//void editorRowDelChar(erow * row, int at, StateMachine *E) {  
//    if (at < 0 || at > row->size) return;
//    memmove(&row->chars[at], &row->chars[at + 1], row->size - at);
//    row->size--;
//    editorUpdateRow(row, E);
//    E->dirty ++;
//}
//
//void editorRowAppendString(erow *row, char *s, size_t len, StateMachine *E) {
//    row->chars = realloc(row->chars, row->size + len + 1);
//    memcpy(&row->chars[row->size], s, len);
//    row->size += len;
//    row->chars[row->size] = '\0';
//    editorUpdateRow(row, E);
//    E->dirty++;
//}

//void editorFreeRow(erow *row) {
//    free(row->render);
//    free(row->chars);
//    free(row->hl);
//}
//
//void editorDelRow(int at, StateMachine *E) {
//    if (at < 0 || at >= E->numRows) return;
//    editorFreeRow(&E->row[at]);
//    memmove(&E->row[at], &E->row[at + 1], sizeof(erow) * (E->numRows - at -1));
//    for (int j = at; j < E->numRows - 1; j++) E->row[j].idx--;
//    E->numRows--;
//    E->dirty++;
//}
//
//void editorDelChar(StateMachine *E ) {
//    if (E->cy == E->numRows) { 
//        return;
//    }
//    if (E->cx ==0 && E->cy ==0) return;
//
//    erow *row = &E->row[E->cy];
//    if (E->cx > 0 ) {
//        editorRowDelChar(row, E->cx -1, E);
//        E->cx --;
//    } else {
//        E->cx = E->row[E->cy - 1].size;
//        editorRowAppendString(&E->row[E->cy - 1], row->chars, row->size, E);
//        editorDelRow(E->cy, E);
//        E->cy--;
//
//    }
//}

void editorRowInsertChar(int key, StateMachine *E) {
    erow *row = &E->row[E->cy];
    int at = E->cx;
    if (at < 0 || at > row->size) at = row->size;
    row->chars = realloc(row->chars, row->size + 2);
    memmove(&row->chars[at + 1], &row->chars[at], row->size - at + 1);
    row->size++;
    row->chars[at] = key;
    editorUpdateRow(row, E);
    E->dirty ++;
}



int editorInsertChar(int key, StateMachine *E) {
    if (E->cy == E->numRows) { 
        editorInsertRow(E->numRows,"", 0, E);
    }
    editorRowInsertChar(key, E);
    E->cx++;
}

int editorInsertNewLine(int key, StateMachine *E) {
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
