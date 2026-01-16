/* main.c */

/* 0 copyright/licensing */

/* 1 includes */
#include "../include/StateMachine.h"

/* 2 defines */
#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE
#define MAD_INC_EDITOR_VERSION "0.0.1"

/* 3 external declarations */

/* 4 typedefs */

/* 5 global variable declarations */

/* 6 function prototypes */

int main (int argc, char *argv[])
{
    StateMachine M;
    M.args.c = argc;
    M.arg.v = argv;
    init(&M);
    run(&M);
    return 0;    
}

void init(StateMachine *M) 
{
    enableRawMode();    
//    initEditor(M);
//    if (argc >=2 ) {
//        editorOpen( argv[1], M);
//    } else {
//        // filePath(&config);
//    }
}

void rum(StateMachine *M)
{ 
    return;
    while (1){
//        editorRefreshScreen(M);
//        editorProcessKeypress(M);
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

void initOutputData(State * E) {
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
