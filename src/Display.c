// this file adds the data to the terminal 
// - Including the syntax
// - cursor position and shape
// - status of the editor 
// - text data  
#include "../include/StateMachine.h"
#include "../include/Display.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

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
//void die(const char*s) {
//    write(STDOUT_FILENO, "\x1b[2j", 4);
//    write(STDOUT_FILENO, "\x1b[H", 3);
//    perror(s);
//    exit(1);
//}



int editorSyntaxToColor(int hl) {
    switch (hl) {
        case HL_COMMENT:
        case HL_MLCOMMENT: return  36;
        case HL_KEYWORD1: return  33;
        case HL_KEYWORD2: return  32;
        case HL_STRING: return  35;
        case HL_NUMBER: return  31;
        case HL_MATCH: return 34;
        default: return 37;
        }
}

void abAppend(struct abuf *ab, const char *s, int len) {
    char *new = realloc(ab->b, ab->len + len);

    if (new == NULL) return;
    memcpy(&new[ab->len], s, len);
    ab ->b = new;
    ab ->len += len;
}


void abFree(struct abuf *ab) {
    free(ab->b);
}

int editorRowCxToRx(erow *row, int cx) {
    int rx = 0;
    int j;
    for (j = 0; j < cx; j++) {
        if (row ->chars[j] == '\t')
            rx += (TAB_STOP - 1) - (rx % TAB_STOP);
        rx ++;
    }
    return rx;
}

void editorScroll(StateMachine *E ) {
    E->rx = E->cx;
    if (E->cy < E->numRows) {
        E->rx = editorRowCxToRx(&E->row[E->cy], E->cx);
    } 

    if (E->cy < E->rowOff) {
        E->rowOff = E->cy;
    }

    if (E->cy >= E->rowOff + E->screenRows) {
        E->rowOff = E->cy - E->screenRows + 1;
    }


    if (E->rx < E->colOff) {
        E->colOff = E->rx;
    }

    if (E->rx >= E->colOff + E->screenCols) {
        E->colOff= E->rx - E->screenCols + 1;
    }
}

void editorDrawEmptyRow(struct abuf *ab) 
{
    abAppend(ab, "~", 1);
}

void terminalClearToEndLine(struct abuf *ab)
{
    abAppend(ab, "\x1b[K", 3);
}

void terminalNewLine(struct abuf *ab) 
{
    abAppend(ab, "\r\n", 2);
}

void editorNewRow(struct abuf *ab) 
{
    terminalClearToEndLine(ab);
    terminalNewLine(ab);
}


void editorDrawNoFile(struct abuf *ab, StateMachine *E)
{
    for (int y = 0; y < E->screenRows; y++) {
        if (E->screenRows/3 == y) {
            char Welcome[80];
            int welcomeLen = snprintf(Welcome, sizeof(Welcome),
                "%s", WELCOME_MESSAGE);
            if (welcomeLen> E->screenCols) welcomeLen = E->screenCols; 
            int padding = (E->screenCols - welcomeLen) / 2;
            if (padding) {
                editorDrawEmptyRow(ab);
                padding--;
            }
            while (padding--) abAppend(ab, " ", 1);
            abAppend(ab, Welcome, welcomeLen);
        } else {
            editorDrawEmptyRow(ab);
        }
        editorNewRow(ab);
    }
}

void editorDrawRow(struct abuf *ab,StateMachine *E, int filerow )
{
    int len = E->row[filerow].rsize - E->colOff;
    if (len < 0) len = 0;
    if (len > E->screenCols) len = E->screenCols;
    char *c =&E->row[filerow].render[E->colOff];
    unsigned char *hl = &E->row[filerow].hl[E->colOff];
    int current_color = -1;
    int j;
    for (j = 0; j < len; j++ ) {
        if (iscntrl(c[j])) 
        {
            char sym = (c[j] <= 26 ) ? '@' + c[j] : '?';
            abAppend(ab, "\x1b[7m", 4);
            abAppend(ab, &sym, 1);
            abAppend(ab, "\x1b[m", 3);
//          if (current_color != -1) 
//          {
//              char buf[16];
//              int clen = snprintf(buf, sizeof(buf), "\x1b[%dm", current_color);
//              abAppend(ab, buf, clen);
//          }
//      } else if ( hl[j] == HL_NORMAL) {
//          if (current_color != -1) 
//          {
//              abAppend(ab, "\x1b[39m", 5);
//              current_color = -1;
//          }
//          abAppend(ab, &c[j],1);
        } else 
        {
//          int color = editorSyntaxToColor(hl[j]);
//          if (color != current_color) 
//          {
//              current_color = color;
//              char buf[16];
//              int clen = snprintf(buf, sizeof(buf), "\x1b[%dm", color);
//              abAppend(ab, buf, clen);
//          }
            abAppend(ab, &c[j], 1); // write character
        }
    }
    abAppend(ab, "\x1b[39m", 5);
}

void editorDrawFile(struct abuf *ab, StateMachine *E) 
{
    for (int y = 0; y < E->screenRows; y++) {
        int filerow = y + E->rowOff;
        if (filerow >= E->numRows)
        {
            editorDrawEmptyRow(ab);
        } else {
            editorDrawRow(ab, E, filerow);

        }
        editorNewRow(ab);
    }
}

void editorDrawRows(struct abuf *ab, StateMachine *E) 
{
    if (E->numRows == 0) 
    {
        editorDrawNoFile(ab, E);
    } else {
       editorDrawFile(ab, E);

    }

}

const char * const MODE_NAMES[] ={
    "NORMAL",
    "VISUAL",
    "INSERT",
    "NAVIGATION",
};


void editorDrawStatusBar(struct abuf *ab, StateMachine *E ) {
    abAppend(ab, "\x1b[7m", 4); // Swaps to Inverted Colors
    char status[80], rstatus[80];
    char modeName[5];



    int len = snprintf(status, sizeof(status), "%.20s - %d lines %s --%s--",
        E->fileName ? E->fileName : "[No Name]",
        E->numRows, 
        E->dirty ? "(changed)" : "",
        MODE_NAMES[E->mode]);
    int rlen = snprintf(rstatus, sizeof(rstatus), "%s | %d,%d", /* E->syntax ? E->syntax->filetype : */"no ft", E->cy + 1, E->cx + 1);
    if (len > E->screenCols) len = E->screenCols;
    abAppend(ab, status, len);
    while (len < E->screenCols) {
        if (E->screenCols - len == rlen) {
            abAppend(ab, rstatus, rlen);
            break;
        } else {
            abAppend(ab, " ", 1);
            len++;
        }
    }
    abAppend(ab, "\x1b[m", 3);
    abAppend(ab, "\r\n", 2);
}

void editorDrawMessageBar(struct abuf *ab, StateMachine * E) {
    abAppend(ab, "\x1b[K", 3);
    int msglen = strlen(E->statusMsg);
    if (msglen > E->screenCols) msglen = E->screenCols;
    if (msglen && time(NULL) - E->statusMsg_time < 5) abAppend(ab, E->statusMsg, msglen);
}


void editorRefreshScreen(StateMachine * E){
    editorScroll(E); // Determines the portion of the File to Show basedon cursor movement 

    struct abuf ab = ABUF_INIT;

    abAppend(&ab, "\x1b[?25l", 6); // Hide Cursor
    abAppend(&ab, "\x1b[H", 3); // Position Cursor to Top-Left

    editorDrawRows(&ab, E);
    editorDrawStatusBar(&ab, E); 
    editorDrawMessageBar(&ab, E);

    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (E->cy - E->rowOff) + 1,(E->rx - E->colOff) + 1);
    abAppend(&ab, buf, strlen(buf)); // Position Cursor

    abAppend(&ab, "\x1b[?25h", 6); // Show Cursor
    abAppend(&ab, "\x1b[6 q", 6); // Sets Cursor to a Line 
    
    write(STDOUT_FILENO, ab.b, ab.len);
    abFree(&ab);
}

int is_separator(int c) {
    return isspace(c) || c == '\0' || strchr(",.()+-/*=~%<>[];", c) != NULL;
}

//void editorUpdateSyntax(erow *row, StateMachine *E) {
//    row->hl = realloc(row->hl, row->rsize);
//    memset(row->hl, HL_NORMAL, row->rsize);
//
//    if (E->syntax == NULL) return;
//
//    char **keywords = E->syntax->keywords;
//
//    char *scs = E->syntax->singleline_comment_start;
//    char *mcs = E->syntax->multiline_comment_start;
//    char *mce = E->syntax->multiline_comment_end;
//
//    int scs_len = scs ? strlen(scs) : 0;
//    int mcs_len = mcs ? strlen(mcs) : 0;
//    int mce_len = mce ? strlen(mce) : 0;
//
//    int prev_sep = 1; 
//    int in_string = 0;
//    int in_comment= (row->idx > 0 && E->row[row->idx - 1].hl_open_comment);
//
//    int i = 0;
//    while (i < row->rsize) {
//        char c = row->render[i];
//        unsigned char prev_hl = (i > 0) ? row->hl[i - 1] : HL_NORMAL;
//
//        if (scs_len && !in_string && !in_comment) {
//            if (!strncmp(&row->render[i], scs, scs_len)) {
//                memset(&row->hl[i], HL_COMMENT, row->rsize - i);
//                break;
//            }
//        }
//
//
//        if (mcs_len && mce_len && !in_string) {
//            if (in_comment) {
//                row->hl[i] = HL_MLCOMMENT;
//                if (!strncmp(&row->render[i], mce, mce_len)) {
//                    memset(&row->hl[i], HL_MLCOMMENT, mce_len);
//                    i += mce_len;
//                    in_comment = 0;
//                    prev_sep = 1;
//                    continue;
//                } else {
//                    i++;
//                    continue;
//                }
//            } else if (!strncmp(&row->render[i], mcs,mcs_len)) {    
//                memset(&row->hl[i], HL_MLCOMMENT, mcs_len);
//                i += mcs_len;
//                in_comment = 1;
//                continue;
//            }
//        }
//        
//        if (E->syntax->flags & HL_HIGHLIGHT_STRINGS) {
//            if (in_string) {
//                row->hl[i] = HL_STRING;
//                if (c == '\\' && i + 1 < row->rsize) {
//                    row->hl[i + 1] = HL_STRING;
//                    i += 2;
//                    continue;
//
//                }
//                if (c == in_string) in_string = 0;
//                i++;
//                prev_sep = 1;
//                continue;
//
//            } else {
//                if (c == '"' || c == '\'') {
//                    in_string = c;
//                    row->hl[i] = HL_STRING;
//                    i++;
//                    continue;
//                }
//            }
//        }
//
//        if (E->syntax->flags & HL_HIGHLIGHT_NUMBERS) {
//            if ((isdigit(c) && (prev_sep || prev_hl == HL_NUMBER)) || (c == '.' && prev_hl == HL_NUMBER)) {
//                row->hl[i] = HL_NUMBER;
//                i++;
//                prev_sep = 0;
//                continue;
//
//            }
//        }
//
//        if (prev_sep) {
//            int j;
//            for (j = 0; keywords[j]; j++) {
//                int klen = strlen(keywords[j]);
//                int kw2 = keywords[j][klen - 1] == '|';
//                if (kw2) klen --;
//
//                if (!strncmp(&row->render[i], keywords[j], klen) && is_separator(row->render[i + klen])) {
//                    memset (&row->hl[i], kw2 ? HL_KEYWORD2 : HL_KEYWORD1, klen);
//                    i += klen;
//                    break;
//                }
//            }
//            if (keywords[j] != NULL) {
//                prev_sep = 0;
//                continue;
//            } 
//        }
//
//        prev_sep = is_separator(c);
//        i++;
//    }
//
//    int changed = (row->hl_open_comment != in_comment);
//    row->hl_open_comment = in_comment;
//    if (changed && row->idx + 1 < E->numRows)
//        editorUpdateSyntax(&E->row[row->idx + 1],  E);
//}

//void editorSelectSyntaxHighlight(StateMachine *E) {
//    E->syntax = NULL;
//    if (E->fileName == NULL) return;
//
//    char *ext = strrchr(E->fileName, '.');
//
//    for (unsigned int j = 0; j < HLDB_ENTRIES; j++) {
//        struct editorSyntax *s = &HLDB[j];
//        unsigned int i = 0;
//        while (s->filematch[i]) {
//            int is_ext = (s->filematch[i][0] == '.');
//            if ((is_ext && ext && !strcmp(ext, s->filematch[i])) || (!is_ext && strstr(E->fileName, s->filematch[i]))) {
//                E->syntax = s;
//
//                int filerow;
//                for (filerow = 0; filerow < E->numRows; filerow++) {
//                    editorUpdateSyntax(&E->row[filerow], E);
//                }
//
//                return;
//            }
//            i++;
//        }
//    }
//}
