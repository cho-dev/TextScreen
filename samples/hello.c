/***************************************
 hello.c
     using TextScreen sample.
     (c)2016 programming by Coffey.
     Date: 20160430
 build command
 (Windows) gcc hello.c textscreen.c -lm -o hello.exe
 (Linux  ) gcc hello.c textscreen.c -lm -o hello.out
****************************************/

#include "textscreen.h"

int main(void)
{
    TextScreenBitmap  *bitmap;
    int  key = 0;
    
    TextScreen_Init(0);                      // Initialize
    bitmap = TextScreen_CreateBitmap(0, 0);  // create bitmap same size of screen
    TextScreen_ClearScreen();                // clear console
    
    TextScreen_DrawText(bitmap, 2, 2, "Hello World !");  // draw text at (2,2) of bitmap
    TextScreen_DrawText(bitmap, 2, 4, "Press [q] or [Esc] to exit");  // draw text at (2,4) of bitmap
    TextScreen_ShowBitmap(bitmap, 0, 0);     // show bitmap at (0,0) of screen
    
    while( key != 'q' && key != TSK_ESC ) {
        key = TextScreen_GetKey() & TSK_KEYMASK;  // get key code without waiting
    }
    
    TextScreen_ClearScreen();                // clear console
    TextScreen_FreeBitmap(bitmap);           // free bitmap
    TextScreen_End();                        // end of TextScreen
    return 0;
}

