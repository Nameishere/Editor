#include "../include/KeyProcess.h"
#include "../include/KeyPressFunctions.h"

static modeData normal = {
    .elseCallback = doNothing,
    .keyCount = 19,
    .keys = {
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
    }
};

static modeData insert = {
    .elseCallback = editorInsertChar,
    .keyCount = 4,
    .keys = {
        {ESC_KEY, editorToNormalMode},
        {CTRL_KEY('q'), editorQuitApp},
        {(int)'\r', editorInsertNewLine},
        {HOME_KEY, doNothing},
    }
};

void editorProcessKeypress(StateMachine* E)
{
    int c = editorReadKey();
    switch (E->mode){
        case MODE_NORMAL:
            for (int i = 0; i < normal.keyCount; i++) {
                if (c == normal.keys[i].c) {
                    normal.keys[i].callback(c, E); 
                    break;
                }
                normal.elseCallback(c,E);
            }

            break;
        case MODE_INSERT:
            for (int i = 0; i < insert.keyCount; i++) {
                if (c == insert.keys[i].c) {
                    insert.keys[i].callback(c, E); 
                    break;
                }
            }
            insert.elseCallback(c,E);
            break;
    }
}

//void normalModeKeyPresses(StateMachine *E, int c) {
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
//
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
