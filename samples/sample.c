/***************************************
 sample.c
     using TextScreen sample.
     (c)2015-2016 programming by Coffey.
****************************************/
// build command (sample.c is this sample)
// (Windows) gcc sample.c textscreen.c -lm -o sample.exe
// (Linux  ) gcc sample.c textscreen.c -lm -o sample.out

#include "textscreen.h"

int main(void)
{
    TextScreenBitmap   *bitmap, *sprite;      // bitmap pointer
    int  x, y, xd, yd, key;
    const char *helptext =  "Press [q] or [Esc] to exit";
    
    TextScreen_Init(0);                       // Initialize
    TextScreen_SetSpaceChar('.');             // set space char to '.'
    bitmap = TextScreen_CreateBitmap(0, 0);   // create bitmap same size of screen
    sprite = TextScreen_CreateBitmap(17, 9);  // create sprite bitmap. size(17,9)
    TextScreen_DrawFillCircle(sprite, 8, 4, 4, '$');  // draw circle. center(8,4) r=4
    TextScreen_ClearScreen();                 // clear console
    
    x   = 5;    // initial position x
    y   = 5;    // initial position y
    xd  = 2;    // stride x
    yd  = 1;    // stride y
    key = 0;
    while( key != 'q' && key != TSK_ESC ) {
        TextScreen_ClearBitmap(bitmap);       // clear bitmap
        TextScreen_OverlayBitmap(bitmap, sprite, x, y);  // copy sprite to bitmap
        TextScreen_DrawText(bitmap, 0, bitmap->height - 1, helptext);  // draw text
        TextScreen_ShowBitmap(bitmap, 0, 0);  // show bitmap to console
        TextScreen_Wait(100);                 // wait 100ms
        x += xd;
        y += yd;
        // bounding check
        if ((x > bitmap->width - sprite->width) || (x < 0)) {
            xd = -xd;
            x += xd + xd;
        }
        if ((y > bitmap->height - sprite->height) || (y < 0)) {
            yd = -yd;
            y += yd + yd;
        }
        key = TextScreen_GetKey() & TSK_KEYMASK;  // get key code without waiting
    }
    TextScreen_FreeBitmap(sprite);            // free sprite
    TextScreen_FreeBitmap(bitmap);            // free bitmap
    TextScreen_End();                         // end of TextScreen
    return 0;
}

