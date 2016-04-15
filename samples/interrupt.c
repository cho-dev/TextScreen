/*****************************************
 interrupt.c
 
 Sample of Ctrl+C action for TextScreen
     (c)2016 Programming by Coffey
     Date: 20160415
 
 build command
 (Windows) gcc interrupt.c textscreen.c -lm -o interrupt.exe
 (Linux  ) gcc interrupt.c textscreen.c -lm -o interrupt.out
 also easy to build with MSVC
 *****************************************/

#include <stdio.h>

#include "textscreen.h"

void sigint_handler(int sig, void *userdata)
{
    char *mes;
    
    // show message use with userdata
    mes = (char *)userdata;
    printf("%s\n", mes);
}

int main(void)
{
    int key = 0;
    char *mes = "[Ctrl]+[C] was pressed !!";
    
    // initialize TextScreen
    TextScreen_Init(0);
    // set SIGINT handler. set a message that will send to sigint_handler.
    // (Linux, Windows both can use same function)
    TextScreen_SetSigintHandler(sigint_handler, (void *)mes);
    
    printf("Press [q] or [Esc] to exit. Press [Ctrl]+[C] to show the message.\n");
    
    // main loop
    while ((key != 'q') && (key != TSK_ESC)) {
        TextScreen_Wait(20);
        key = TextScreen_GetKey();
    }
    
    // show key that was pressed.
    if (key == 'q') {
        printf("[q] was pressed. ");
    }
    if (key == TSK_ESC) {
        printf("[Esc] was pressed. ");
    }
    printf("Program is terminated.\n");
    
    // release TextScreen
    TextScreen_End();
    return 0;
}

