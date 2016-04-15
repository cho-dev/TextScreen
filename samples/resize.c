/*****************************************
 resize.c
 
 Sample of follow the resizing console.
     (c)2016 Programming by Coffey
     Date: 20160416
 
 build command
 (Windows) gcc resize.c textscreen.c -lm -o resize.exe
 (Linux  ) gcc resize.c textscreen.c -lm -o resize.out
 also easy to build with MSVC
 *****************************************/

#include <stdio.h>

#include "textscreen.h"

int main(void)
{
    TextScreenBitmap *bitmap;
    int  width, height;
    int  consoleWidth, consoleHeight;
    int  key, redraw;
    char *helptext = "[q] or [Esc] to exit. Try change window size.";
    
    // initialize TextScreen
    TextScreen_Init(0);
    bitmap = TextScreen_CreateBitmap(0, 0);
    
    // get current console size
    TextScreen_GetConsoleSize(&consoleWidth, &consoleHeight);
    
    // main loop
    key = 0;
    redraw = 1;
    while ((key != 'q') && (key != TSK_ESC)) {  // q or esc then quit
        // check resize console
        TextScreen_GetConsoleSize(&width, &height);
        if ((width != consoleWidth) || (height != consoleHeight)) {
            TextScreen_ResizeScreen(0, 0);
            TextScreen_FreeBitmap(bitmap);
            bitmap = TextScreen_CreateBitmap(0, 0);
            consoleWidth  = width;
            consoleHeight = height;
            redraw = 1;
        }
        // draw box and help text
        if (redraw) {
            TextScreen_ClearBitmap(bitmap);
            TextScreen_DrawRect(bitmap, 0, 0, bitmap->width, bitmap->height - 1, '$', 0);
            TextScreen_DrawLine(bitmap, 0, 0, bitmap->width - 1, bitmap->height - 2, '$');
            TextScreen_DrawLine(bitmap, 0, bitmap->height - 2, bitmap->width - 1, 0, '$');
            TextScreen_DrawText(bitmap, 0, bitmap->height - 1, helptext);
            
            TextScreen_ClearScreen();
            TextScreen_ShowBitmap(bitmap, 0, 0);
            redraw = 0;
        }
        // wait 50ms
        TextScreen_Wait(50);
        // get key code from key buffer
        key = TextScreen_GetKey() & TSK_KEYMASK;
    }
    
    // move cursor to bottom
    TextScreen_SetCursorPos(0, consoleHeight - 1);
    printf("\n");
    // free bitmap
    TextScreen_FreeBitmap(bitmap);
    // release TextScreen
    TextScreen_End();
    
    return 0;
}

