/*****************************************
 rectbench.c
 
 Drawing benchmark.
     (c)2016 Programming by Coffey
     Date: 20160421
 
 build command
 (Windows) gcc rectbench.c textscreen.c -lm -o rectbench.exe
 (Linux  ) gcc rectbench.c textscreen.c -lm -o rectbench.out
 *****************************************/

#include <stdio.h>

#include "textscreen.h"

int main(void)
{
    TextScreenBitmap *bitmap;
    int i, j;
    unsigned int ticks;
    
    // init TextScreen
    TextScreen_Init(0);
    bitmap = TextScreen_CreateBitmap(0, 0);
    TextScreen_ClearScreen();
    
    // benchmark start (draw fill rectangle)
    ticks = TextScreen_GetTickCount();
    for (j = 0; j < 100; j++) {
        for (i = 'A'; i <= 'Z'; i++) {
            TextScreen_DrawFillRect(bitmap, 0, 0, bitmap->width, bitmap->height, (char)i);
            TextScreen_ShowBitmap(bitmap, 0, 0);
        }
    }
    ticks = TextScreen_GetTickCount() - ticks;
    
    {  // print result
        int  width, height;
        int  charPerRect, numRect;
        
        width  = bitmap->width;
        height = bitmap->height;
        charPerRect = width * height;
        numRect = j * ('Z' - 'A' + 1);
        
        printf("\n\n");
        printf("Fill Rectangle (%dx%d) x %d times.\n", width, height, numRect);
        printf("Time : %d.%03d sec (%.2f rect/sec, %.0f chars/sec)\n\n", ticks / 1000, ticks % 1000,
                (double)numRect * 1000 / ticks, (double)charPerRect * numRect * 1000 / ticks);
    }
    TextScreen_FreeBitmap(bitmap);
    TextScreen_End();
    
    return 0;
}
