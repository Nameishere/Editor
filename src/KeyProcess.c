#include "../include/KeyProcess.h"
#include "../include/KeyPressFunctions.h"

static keyMapping normalKeys[] = {
    {(int)'j', editorMoveCursorDown},
    {(int)'k', editorMoveCursorUp},
    {(int)'h', editorMoveCursorLeft},
    {(int)'l', editorMoveCursorRight},
    {CTRL_KEY('q'), editorQuitApp},
    {(int)'i', editorToInsertMode},
    {ARROW_DOWN, editorMoveCursorDown},
    {ARROW_UP, editorMoveCursorUp},
    {ARROW_LEFT, editorMoveCursorLeft},
    {ARROW_RIGHT, editorMoveCursorRight},
    {(int)'H', editorMoveCursorScreenTop},
    {(int)'M', editorMoveCursorScreenMiddle},
    {(int)'L', editorMoveCursorScreenBottom},
    {(int)'W', editorMoveCursorWordStartNP},
    {(int)'w', editorMoveCursorWordStart},
    {(int)'E', editorMoveCursorWordEndNP},
    {(int)'e', editorMoveCursorWordEnd},
    {(int)'B', editorMoveCursorLastWordStartNP},
    {(int)'b', editorMoveCursorLastWordStart},

};

int normalKeysSize = sizeof(normalKeys) / sizeof(normalKeys[0]);

static keyMapping insertKeys[30] = {
    {ESC_KEY, editorToNormalMode},
    {CTRL_KEY('q'), editorQuitApp},
    // {(int)'\r', editorInsertNewLine},
    {HOME_KEY, doNothing},
};

int insertKeysSize = sizeof(insertKeys) / sizeof(insertKeys[0]);

void editorProcessKeypress(OutputData* E){
    int c = editorReadKey();

    switch (E->mode){
        case MODE_NORMAL:
            for (int i = 0; i < normalKeysSize; i++) {
                if (c == normalKeys[i].c) {
                    normalKeys[i].callback(c, E); 
                    break;
                }
            }
            break;
        case MODE_INSERT:
            for (int i = 0; i < insertKeysSize; i++) {
                if (c == insertKeys[i].c) {
                    insertKeys[i].callback(c, E); 
                    break;
                }
            }
            
            break;
    }
}

// void normalModeKeyPresses(OutputData *E, int c) {
//     static int quit_times = QUIT_TIMES;
//     switch(c) {
//         case '\r':
//             editorInsertNewLine(E);
//             break;
//         case CTRL_KEY('q'):
//             break;
//         case CTRL_KEY('s'):
//             break;
//         case HOME_KEY:
//             E->cx = 0;
//             break;
//         case END_KEY:
//             if (E->cy < E->numrows) {
//                 E->cx = E->row[E->cy].size;
//             }
//             break;        
//         case CTRL_KEY('F'):
//             editorFind(E);
//             break;    
//         case BACKSPACE:
//         case CTRL_KEY('h'):
//         case DEL_KEY:
//             if (c == DEL_KEY) editorMoveCursor(ARROW_RIGHT, E);
//             editorDelChar(E);
//             break;
//         case PAGE_UP:
//         case PAGE_DOWN:
//             {
//                 if (c == PAGE_UP) {
//                     E->cy = E->rowoff;
//                 } else if (c == PAGE_DOWN) {
//                     E->cy = E->rowoff + E->screenRows - 1;
//                     if (E->cy > E->numrows) E->cy = E->numrows;
//                 }

//                 int times = E->screenRows;
//                 while (times --) editorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN, E);
//             }
//             break;        
//         case ARROW_LEFT:
//         case ARROW_RIGHT:
//         case ARROW_UP:
//         case ARROW_DOWN:
//             editorMoveCursor(c, E);
//             break;
//         case CTRL_KEY('l'):
//         case '\x1b':
//             break;
//         default:
//             editorInsertChar(c, E);
//             break;
//     }
//     quit_times = QUIT_TIMES;
// }

/* init */