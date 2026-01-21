/* main.c */

/* 0 copyright/licensing */

/* 1 includes */
#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE


#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "../include/StateMachine.h"
#include "../include/KeyProcess.h"
#include "../include/Display.h"

/* 2 defines */
#define MAD_INC_EDITOR_VERSION "0.0.1"

/* 3 external declarations */

/* 4 typedefs */
typedef struct ProgramArgs {
    int c;
    char **v;
}ProgramArgs;

/* 5 global variable declarations */

/* 6 function prototypes */
void init(StateMachine *M, ProgramArgs arg);
void run(StateMachine *M);

int main (int argc, char *argv[])
{
    ProgramArgs arg;
    arg.c = argc;
    arg.v = argv;
    StateMachine M;
    init(&M, arg);
    run(&M);
    return 0;    
}

struct termios org_termios;

void die(const char*s) {
    write(STDOUT_FILENO, "\x1b[2j", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
    perror(s);
    exit(1);
}

void disableRawMode() 
{
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &org_termios) == -1)
    {
        die("tcsetattr");
    }
}

void enableRawMode() {
    if (tcgetattr(STDIN_FILENO, &org_termios) == -1)
    {
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
    if ( tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) 
    {
        die("tcsetattr");
    }
}

int getCursorPosition(int *rows, int *cols) {
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

int getWindowSize( int *rows, int *cols) {
    struct winsize ws;

    if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0 ) {
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
        return getCursorPosition(rows, cols);
    } else {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}

void initStateMachine(StateMachine * M, ProgramArgs arg) 
{
    M->cx = 0;
    M->cy = 0;
    M->rx = 0;
    M->rowOff = 0;
    M->colOff = 0;
    M->numRows = 0;
    M->row = NULL;
    M->fileName = NULL;
    M->statusMsg[0] = '\0';
    M->statusMsg_time = 0;
    M->dirty = 0;
    // M->syntax = NULL;
    M->mode = MODE_NORMAL;

    if (getWindowSize(&M->screenRows, &M->screenCols)== -1) die("getWindowSize");
    M->screenRows -= 2;

    if (arg.c >=2 ) 
    {
        editorOpen(arg.v[1], M); //fix Problem with editor Open 
    } else {
        //filePath(&config);
    }

}

void init(StateMachine *M, ProgramArgs arg) 
{
    enableRawMode();    
    initStateMachine(M, arg);
    editorSetStatusMessage(M,"Start");
}

void run(StateMachine *M)
{ 
    while (1){
        
        editorRefreshScreen(M);
        editorProcessKeypress(M);
    }
    
}

