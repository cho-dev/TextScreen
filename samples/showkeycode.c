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
    TextScreen_Init(NULL);  // initialize TextScreen
    printf("Show TextScreen's keycode. Press key. [q]quit\n");
    while(1) {
        char buf[256];
        int  key;
        
        key = TextScreen_GetKey();
        if (key) {
            sprintf(buf, "key = %x, %x         \n", key, key & TSK_KEYMASK);
            printf("%s",buf);
            
            if (key == 'q') break;
        }
        TextScreen_Wait(50);
    }
    TextScreen_End();  // Free TextScreen
    return 0;
}

