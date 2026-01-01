#include "../include/Output.h"

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
void die(const char*s) {
    write(STDOUT_FILENO, "\x1b[2j", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
    perror(s);
    exit(1);
}

void editorSetStatusMessage(OutputData *E, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(E->statusmsg, sizeof(E->statusmsg), fmt, ap);
  va_end(ap);
  E->statusmsg_time = time(NULL);
}


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

void editorScroll(OutputData *E ) {
    E->rx = E->cx;
    if (E->cy < E->numrows) {
        E->rx = editorRowCxToRx(&E->row[E->cy], E->cx);
    } 

    if (E->cy < E->rowoff) {
        E->rowoff = E->cy;
    }

    if (E->cy >= E->rowoff + E->screenRows) {
        E->rowoff = E->cy - E->screenRows + 1;
    }


    if (E->rx < E->coloff) {
        E->coloff = E->rx;
    }

    if (E->rx >= E->coloff + E->screenCols) {
        E->coloff= E->rx - E->screenCols + 1;
    }
}

void editorDrawRows(struct abuf *ab, OutputData *E) {
    int y; 
    for (y = 0; y < E->screenRows; y++) {
        int filerow = y + E->rowoff;
        if (filerow >= E->numrows) {
            if (E->numrows == 0 && y == E->screenRows / 3) {
                char Welcome[80];
                int welcomeLen = snprintf(Welcome, sizeof(Welcome),
                    "%s", WELCOME_MESSAGE);
                if (welcomeLen> E->screenCols) welcomeLen = E->screenCols; 
                int padding = (E->screenCols - welcomeLen) / 2;
                if (padding) {
                    abAppend(ab, "~", 1 );
                    padding--;
                }
                while (padding--) abAppend(ab, " ", 1);
                abAppend(ab, Welcome, welcomeLen);
            } else {
                abAppend(ab, "~", 1);
            } 
        } else {
            int len = E->row[filerow].rsize - E->coloff;
            if (len < 0) len = 0;
            if (len > E->screenCols) len = E->screenCols;
            char *c =&E->row[filerow].render[E->coloff];
            unsigned char *hl = &E->row[filerow].hl[E->coloff];
            int current_color = -1;
            int j;
            for (j = 0; j < len; j++ ) {
                if (iscntrl(c[j])) {
                    char sym = (c[j] <= 26 ) ? '@' + c[j] : '?';
                        abAppend(ab, "\x1b[7m", 4);
                        abAppend(ab, &sym, 1);
                        abAppend(ab, "\x1b[m", 3);
                        if (current_color != -1) {
                            char buf[16];
                            int clen = snprintf(buf, sizeof(buf), "\x1b[%dm", current_color);
                            abAppend(ab, buf, clen);
                        }
                } else if ( hl[j] == HL_NORMAL) {
                    if (current_color != -1) {
                        abAppend(ab, "\x1b[39m", 5);
                        current_color = -1;

                    }
                    abAppend(ab, &c[j],1);
                } else {
                    int color = editorSyntaxToColor(hl[j]);
                    if (color != current_color) {
                        current_color = color;
                        char buf[16];
                        int clen = snprintf(buf, sizeof(buf), "\x1b[%dm", color);
                        abAppend(ab, buf, clen);

                    }
                    abAppend(ab, &c[j], 1);

                }
            }
            abAppend(ab, "\x1b[39m", 5);

        }

        abAppend(ab, "\x1b[K", 3);
        abAppend(ab, "\r\n", 2);
    }
}

const char * const MODE_NAMES[] ={
    "NORMAL",
    "VISUAL",
    "INSERT",
    "NAVIGATION",
};


void editorDrawStatusBar(struct abuf *ab, OutputData *E ) {
    abAppend(ab, "\x1b[7m", 4); // Swaps to Inverted Colors
    char status[80], rstatus[80];
    char modeName[5];



    int len = snprintf(status, sizeof(status), "%.20s - %d lines %s --%s--",
        E->filename ? E->filename : "[No Name]",
        E->numrows, 
        E->dirty ? "(changed)" : "",
        MODE_NAMES[E->mode]);
    int rlen = snprintf(rstatus, sizeof(rstatus), "%s | %d/%d",
        E->syntax ? E->syntax->filetype : "no ft", E->cy + 1, E->numrows);
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

void editorDrawMessageBar(struct abuf *ab, OutputData * E) {
    abAppend(ab, "\x1b[K", 3);
    int msglen = strlen(E->statusmsg);
    if (msglen > E->screenCols) msglen = E->screenCols;
    if (msglen && time(NULL) - E->statusmsg_time < 5)
        abAppend(ab, E->statusmsg, msglen);
}


void editorRefreshScreen(OutputData * E){
    editorScroll(E);

    struct abuf ab = ABUF_INIT;

    abAppend(&ab, "\x1b[?25l", 6);
    abAppend(&ab, "\x1b[H", 3);

    editorDrawRows(&ab, E);
    editorDrawStatusBar(&ab, E);
    editorDrawMessageBar(&ab, E);

    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (E->cy - E->rowoff) + 1,(E->rx - E->coloff) + 1);
    abAppend(&ab, buf, strlen(buf));

    abAppend(&ab, "\x1b[?25h", 6);
    
    write(STDOUT_FILENO, ab.b, ab.len);
    abFree(&ab);
}

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