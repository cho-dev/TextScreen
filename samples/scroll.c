/*****************************************
 scroll.c
 
 Read this source and show.
     (c)2016 Programming by Coffey
     Date: 20160416
 
 build command
 (Windows) gcc scroll.c textscreen.c -lm -o scroll.exe
 (Linux  ) gcc scroll.c textscreen.c -lm -o scroll.out
 also easy to build with MSVC
 *****************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "textscreen.h"

// --------------------------------------
// read file "scroll.c"
// --------------------------------------
char *read_file(void)
{
    FILE *fp;
    char *buffer;
    int  memsize = 8192;
    int  pos;
    
    buffer = (char *)malloc(memsize);
    if (buffer == NULL)
        return NULL;
    
    fp = fopen("scroll.c", "r");
    if (!fp) {
        printf("scroll.c was not found\n");
        free(buffer);
        return NULL;
    }
    pos = 0;
    while (1) {
        int c;
        
        c = fgetc(fp);
        if (c == EOF) break;
        buffer[pos++] = c;
        if (pos >= memsize - 1) break;
    }
    buffer[pos] = 0;
    fclose(fp);
    
    return buffer;
}

int main(void)
{
    TextScreenBitmap *screenmap, *docmap;
    char *doc;
    int  max_line, max_col;
    
    // read file "scroll.c"
    doc = read_file();
    if (doc == NULL) {
        printf("Error: Read [scroll.c] failed.\n");
        return -1;
    }
    
    // check max line and max column
    {
        int  i;
        char *p;
        
        p = doc;
        i = 0;
        max_col  = 0;
        max_line = 1;
        while (*p) {
            if (*p == 0x0a) {
                if (max_col < i)
                    max_col = i;
                max_line++;
                i = 0;
            } else if (*p != 0x0d) {
                i++;
            }
            p++;
        }
    }
    
    // initialize TextScreen
    TextScreen_Init(0);
    
    // make bitmap
    docmap = TextScreen_CreateBitmap(max_col, max_line);
    screenmap = TextScreen_CreateBitmap(0, 0);
    
    // draw document to bitmap
    {
        int  x, y;
        char *p;
        
        x = 0;
        y = 0;
        p = doc;
        while (*p) {
            if (*p == 0x0a) {
                y++;
                x = 0;
            } else if (*p != 0x0d) {
                TextScreen_PutCell(docmap, x++, y, *p);
            }
            p++;
        }
    }
    
    // clear screen
    TextScreen_ClearScreen();
    TextScreen_SetCursorVisible(0);
    
    // main loop
    {
        int  x, y;
        int  key;
        int  redraw, showlineno;
        char *helptext = "[arrow key]scroll view  [home]reset view  [l]line No.  [q][Esc]exit";
        
        // draw to screen
        x = 0;
        y = 0;
        key = 0;
        redraw = 1;
        showlineno = 0;
        // main loop
        while ((key != 'q') && (key != TSK_ESC)) {  // q or esc then quit
            if (key) {
                switch (key) {
                    case 'l':
                        showlineno = !(showlineno);
                        break;
                    case TSK_ARROW_UP:     // up arrow
                        y++;
                        break;
                    case TSK_ARROW_DOWN:   // down arrow
                        y--;
                        break;
                    case TSK_ARROW_LEFT:   // left arrow
                        x++;
                        break;
                    case TSK_ARROW_RIGHT:  // right arrow
                        x--;
                        break;
                    case TSK_HOME:         // home key
                        x = 0;
                        y = 0;
                        break;
                    default:
                        break;
                }
                redraw = 1;
            }
            if (redraw) {
                int  i;
                char numstr[16];
                
                // draw to screen
                TextScreen_ClearBitmap(screenmap);
                if (showlineno) {  //  0:hide line number  1:show line number
                    TextScreen_OverlayBitmap(screenmap, docmap, x + 8, y);
                    for (i = 0; i < screenmap->height - 1; i++) {
                        if ((i - y + 1 > 0) && (i - y + 1 <= max_line)) {
                            snprintf(numstr, sizeof(numstr), "%5d : ", i - y + 1);
                            TextScreen_DrawText(screenmap, 0, i, numstr);
                        }
                    }
                } else {
                    TextScreen_OverlayBitmap(screenmap, docmap, x, y);
                }
                TextScreen_DrawText(screenmap, 0, screenmap->height - 1, helptext);
                TextScreen_ShowBitmap(screenmap, 0, 0);
                redraw = 0;
            }
            // wait 20ms
            TextScreen_Wait(20);
            // get key code from key buffer
            key = TextScreen_GetKey() & TSK_KEYMASK;
        }
    }
    
    // show cursor and clear screen
    TextScreen_SetCursorVisible(1);
    TextScreen_ClearScreen();
    
    // free memory
    free(doc);
    // free bitmap
    TextScreen_FreeBitmap(docmap);
    TextScreen_FreeBitmap(screenmap);
    // release TextScreen
    TextScreen_End();
    
    return 0;
}

