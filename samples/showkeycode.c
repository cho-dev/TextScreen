// --------------------------------------------
// show key code of TextScreen. sample.
// 
// build command:
// gcc showkeycode.c textscreen.c -Wall -lm -o showkeycode.exe
// --------------------------------------------

#include <string.h>
#include <stdio.h>
#include "textscreen.h"

int main(void)
{
    int  key = 0;
    
    TextScreen_Init(NULL);  // initialize TextScreen
    printf("Show TextScreen's keycode. Press key. [q]quit\n");
    while (key != 'q') {
        key = TextScreen_GetKey();
        if (key) {
            printf("key = %x, %x         \n", key, key & TSK_KEYMASK);
            if (key == 'q') break;
        }
        TextScreen_Wait(50);
    }
    TextScreen_End();  // Free TextScreen
    return 0;
}

