/**********************************************************
 conway's game of life
     programming by Coffey   20151030
               last modified 20160413
     for Windows, Linux(ubuntu)
 require:
 textscree.c, textscreen.h (version >= 20160406)
 
 build:
 gcc life.c textscreen.c -Wall -lm -o life.exe (Windows)
 gcc life.c textscreen.c -Wall -lm -o life.out (Linux)
 **********************************************************/
// use snprintf
// #define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
// include windows.h for Clipboard API
#include <windows.h>
#endif

// default board size
#define BOARD_SIZE_WIDTH   500
#define BOARD_SIZE_HEIGHT  500
// version string
#define VERSION_STR        "1.44"

#define BOARD_SPACE_CHAR  '.'
#define BOARD_ALIVE_CHAR  '#'

#include "textscreen.h"

#if TEXTSCREEN_TEXTSCREEN_VERSION < 20160406
#error TextScreen version is too old. Build process aborted.
#endif

typedef struct Rect {
    int top;
    int bottom;
    int left;
    int right;
} Rect;

typedef struct LifeParam {
    TextScreenBitmap *bitmap;
    TextScreenBitmap *storebitmap;
    TextScreenBitmap *restartbitmap;
    TextScreenSetting setting;
    int  consoleWidth;
    int  consoleHeight;
    int  offsetx;
    int  offsety;
    int  wait;
    int  genCount;
    int  restartGenCount;
    int  pause;
    int  quit;
    char aliveChar;
    Rect active;
    int  borderless;
    int  rule_alive[9];
    int  rule_born[9];
    char name[256];
} LifeParam;

// -------------------------------------------------------------
// set default rules to LifeParam
// -------------------------------------------------------------
void SetDefaultRules(LifeParam *lp)
{
    // set default rule (B3/S23)
    lp->rule_alive[0] = 0;
    lp->rule_alive[1] = 0;
    lp->rule_alive[2] = 1;
    lp->rule_alive[3] = 1;
    lp->rule_alive[4] = 0;
    lp->rule_alive[5] = 0;
    lp->rule_alive[6] = 0;
    lp->rule_alive[7] = 0;
    lp->rule_alive[8] = 0;
    
    lp->rule_born[0] = 0;
    lp->rule_born[1] = 0;
    lp->rule_born[2] = 0;
    lp->rule_born[3] = 1;
    lp->rule_born[4] = 0;
    lp->rule_born[5] = 0;
    lp->rule_born[6] = 0;
    lp->rule_born[7] = 0;
    lp->rule_born[8] = 0;
}

// -------------------------------------------------------------
// initialize LifeParam
// -------------------------------------------------------------
void InitLifeParam(LifeParam *lp)
{
    int width  = BOARD_SIZE_WIDTH;
    int height = BOARD_SIZE_HEIGHT;
    
    lp->bitmap = TextScreen_CreateBitmap(width, height);
    lp->storebitmap = TextScreen_DupBitmap(lp->bitmap);
    lp->restartbitmap = TextScreen_DupBitmap(lp->bitmap);
    TextScreen_GetConsoleSize(&(lp->consoleWidth), &(lp->consoleHeight));
    lp->offsetx = (width - lp->setting.width) / 2;
    lp->offsety = (height - lp->setting.height) / 2;
    lp->aliveChar = BOARD_ALIVE_CHAR;
    lp->genCount = 0;
    lp->restartGenCount = 0;
    lp->wait = 10;
    lp->pause = 0;
    lp->quit = 0;
    lp->active.top = 0;
    lp->active.bottom = lp->bitmap->height - 1;
    lp->active.left = 0;
    lp->active.right = lp->bitmap->width - 1;
    lp->borderless = 0;
    lp->name[0] = 0;
    
    // set default rule
    SetDefaultRules(lp);
    
    // initial pattern (F-pentomino)
    TextScreen_PutCell(lp->bitmap, width / 2    , height / 2 - 1, lp->aliveChar);
    TextScreen_PutCell(lp->bitmap, width / 2 + 1, height / 2 - 1, lp->aliveChar);
    TextScreen_PutCell(lp->bitmap, width / 2 - 1, height / 2    , lp->aliveChar);
    TextScreen_PutCell(lp->bitmap, width / 2    , height / 2    , lp->aliveChar);
    TextScreen_PutCell(lp->bitmap, width / 2    , height / 2 + 1, lp->aliveChar);
}

// -------------------------------------------------------------
// free memory for LifeParam
// -------------------------------------------------------------
void FreeLifeParam(LifeParam *lp)
{
    TextScreen_FreeBitmap(lp->bitmap);
    TextScreen_FreeBitmap(lp->storebitmap);
    TextScreen_FreeBitmap(lp->restartbitmap);
}

// -------------------------------------------------------------
// detect alive region (rectangle).
// -------------------------------------------------------------
void CheckActive(LifeParam *lp)
{
    int x, y;
    
    lp->active.top = lp->bitmap->height - 1;
    lp->active.bottom = 0;
    lp->active.left = lp->bitmap->width - 1;
    lp->active.right = 0;
    for (y = 0; y < lp->bitmap->height; y++) {
        for (x = 0; x < lp->bitmap->width; x++) {
            if (TextScreen_GetCell(lp->bitmap, x, y) == lp->aliveChar) {
                if (x < lp->active.left) lp->active.left = x;
                if (x > lp->active.right) lp->active.right = x;
                if (y < lp->active.top) lp->active.top = y;
                if (y > lp->active.bottom) lp->active.bottom = y;
            }
        }
    }
    if (lp->active.left > lp->active.right) lp->active.left = lp->active.right;
    if (lp->active.top > lp->active.bottom) lp->active.top = lp->active.bottom;
}

// -------------------------------------------------------------
// make next generation pattern
// -------------------------------------------------------------
void NextGeneration(LifeParam *lp)
{
    TextScreenBitmap *bitmapSrc, *bitmapDst;
    int x, y;
    int xmin, xmax, ymin, ymax;
    
    bitmapSrc = lp->bitmap;
    bitmapDst = TextScreen_CreateBitmap(bitmapSrc->width, bitmapSrc->height);
    
    if (lp->rule_born[0] || lp->borderless) {
        xmin = 0;
        xmax = lp->bitmap->width - 1;
        ymin = 0;
        ymax = lp->bitmap->height - 1;
    } else {
        xmin = lp->active.left - 1;
        xmax = lp->active.right + 1;
        ymin = lp->active.top - 1;
        ymax = lp->active.bottom + 1;
        if (xmin < 0) xmin = 0;
        if (xmax >= lp->bitmap->width) xmax = lp->bitmap->width - 1;
        if (ymin < 0) ymin = 0;
        if (ymax >= lp->bitmap->height) ymax = lp->bitmap->height - 1;
    }
    
    for (y = ymin; y <= ymax; y++) {
        for (x = xmin; x <= xmax + 1; x++) {
            int neighbor;
            if (lp->borderless) {
                int w, h;
                w = lp->bitmap->width;
                h = lp->bitmap->height;
                neighbor = (TextScreen_GetCell(bitmapSrc, (x - 1 + w) % w, (y - 1 + h) % h) == lp->aliveChar)
                         + (TextScreen_GetCell(bitmapSrc, (x + 0 + w) % w, (y - 1 + h) % h) == lp->aliveChar)
                         + (TextScreen_GetCell(bitmapSrc, (x + 1 + w) % w, (y - 1 + h) % h) == lp->aliveChar)
                         + (TextScreen_GetCell(bitmapSrc, (x - 1 + w) % w, (y + 0 + h) % h) == lp->aliveChar)
                         + (TextScreen_GetCell(bitmapSrc, (x + 1 + w) % w, (y + 0 + h) % h) == lp->aliveChar)
                         + (TextScreen_GetCell(bitmapSrc, (x - 1 + w) % w, (y + 1 + h) % h) == lp->aliveChar)
                         + (TextScreen_GetCell(bitmapSrc, (x + 0 + w) % w, (y + 1 + h) % h) == lp->aliveChar)
                         + (TextScreen_GetCell(bitmapSrc, (x + 1 + w) % w, (y + 1 + h) % h) == lp->aliveChar);
            } else {
                neighbor = (TextScreen_GetCell(bitmapSrc, x - 1, y - 1) == lp->aliveChar)
                         + (TextScreen_GetCell(bitmapSrc, x + 0, y - 1) == lp->aliveChar)
                         + (TextScreen_GetCell(bitmapSrc, x + 1, y - 1) == lp->aliveChar)
                         + (TextScreen_GetCell(bitmapSrc, x - 1, y + 0) == lp->aliveChar)
                         + (TextScreen_GetCell(bitmapSrc, x + 1, y + 0) == lp->aliveChar)
                         + (TextScreen_GetCell(bitmapSrc, x - 1, y + 1) == lp->aliveChar)
                         + (TextScreen_GetCell(bitmapSrc, x + 0, y + 1) == lp->aliveChar)
                         + (TextScreen_GetCell(bitmapSrc, x + 1, y + 1) == lp->aliveChar);
            }
            if (TextScreen_GetCell(bitmapSrc, x, y) == lp->aliveChar) {
                if (lp->rule_alive[neighbor]) {
                    TextScreen_PutCell(bitmapDst, x, y, lp->aliveChar);
                }
            } else {
                if (lp->rule_born[neighbor]) {
                    TextScreen_PutCell(bitmapDst, x, y, lp->aliveChar);
                    if (x < lp->active.left) lp->active.left = x;
                    if (x > lp->active.right) lp->active.right = x;
                    if (y < lp->active.top) lp->active.top = y;
                    if (y > lp->active.bottom) lp->active.bottom = y;
                }
            }
        }
    }
    
    TextScreen_FreeBitmap(bitmapSrc);
    lp->bitmap = bitmapDst;
    lp->genCount++;
}

// -------------------------------------------------------------
// Clipboard (Copy) (Clipboard action is currently Windows only)
// -------------------------------------------------------------
int SaveToClipboard(LifeParam *lp)
{
#ifdef _WIN32
    HGLOBAL cdata;
    SIZE_T  size;
    char *p;
    char cell;
    int x, y, index;
    
    if (OpenClipboard( GetConsoleWindow() )) {
        size = (lp->bitmap->width + 2) * lp->bitmap->height + 2;
        cdata = GlobalAlloc(GMEM_MOVEABLE, size);
        if (cdata) {
            p= (char *)GlobalLock(cdata);
            index = 0;
            for (y = 0; y < lp->bitmap->height; y++) {
                for (x = 0; x < lp->bitmap->width; x++) {
                    cell = TextScreen_GetCell(lp->bitmap, x, y);
                    p[index++] = (cell == lp->aliveChar ? '#' : cell);
                }
                p[index++] = 0x0d;
                p[index++] = 0x0a;
            }
            p[index++] = 0x00;
            GlobalUnlock(cdata);
            EmptyClipboard();
            if (SetClipboardData(CF_TEXT, cdata) == NULL) {
                // printf("Err:%d\n", GetLastError());
                GlobalFree(cdata);
            }
        }
        CloseClipboard();
    }
    return 0;
#else
    return 0;
#endif
}

// -------------------------------------------------------------
// Parse RLE format (ignore header)
// The rule assumes B3/S23
// See http://www.conwaylife.com/wiki/RLE
// -------------------------------------------------------------
void ParseRLE(LifeParam *lp, char *rletext)
{
    int index = 0;
    int quit = 0;
    int x = 0;
    int y = 0;
    int snum = 0;
    int i;
    
    while (rletext[index]) {
        switch (rletext[index]) {
            case 0x0d:
            case 0x09:
            case ' ':
                snum = 0;
                break;
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                snum = snum * 10 + (rletext[index] - '0');
                break;
            case 'B':
            case 'b':
                if (snum == 0) snum = 1;
                for (i = 0; i < snum; i++) {
                    x++;
                }
                snum = 0;
                break;
            case 'O':
            case 'o':
                if (snum == 0) snum = 1;
                for (i = 0; i < snum; i++) {
                    TextScreen_PutCell(lp->bitmap, x, y, lp->aliveChar);
                    x++;
                }
                snum = 0;
                break;
            case '$':
                if (snum == 0) snum = 1;
                x = 0;
                y += snum;
                if (y >= lp->bitmap->height) quit = 1;
                snum = 0;
                break;
            case '!':
                quit = 1;
                break;
            case '#':
            case 'X':
            case 'x':
            default:
                while (rletext[index]) {
                    if(rletext[index] == 0x0a) {
                        break;
                    }
                    index++;
                }
                if (rletext[index] == 0) quit = 1;
                break;
        }
        if (quit) break;
        index++;
    }
}

// -------------------------------------------------------------
// parse plain text
// This code can read Plaintext format of  http://www.conwaylife.com/wiki/Main_Page
// and clipboard data made by this program.
// -------------------------------------------------------------
void ParsePlainText(LifeParam *lp, char *plaintext)
{
    int index = 0;
    int quit = 0;
    int x = 0;
    int y = 0;
    
    while (plaintext[index]) {
        switch (plaintext[index]) {
            case 0x0a:
                x = 0;
                y++;
                if (y >= lp->bitmap->height) quit = 1;
                break;
            case 0x0d:
                break;
            case 'O':   // for http://www.conwaylife.com/wiki/Plaintext
            case '#':
                TextScreen_PutCell(lp->bitmap, x, y, lp->aliveChar);
                x++;
                break;
            case '!':
                while (plaintext[index]) {
                    if (plaintext[index] == 0x0a) {
                        x = 0;
                        y++;
                        if (y >= lp->bitmap->height) quit = 1;
                        break;
                    }
                    index++;
                }
                if (plaintext[index] == 0) quit = 1;
                break;
            default:
                x++;
                break;
        }
        if (quit) break;
        index++;
    }
}

// -------------------------------------------------------------
// Clipboard (Paste) (Clipboard action is currently Windows only)
// -------------------------------------------------------------
int LoadFromClipboard(LifeParam *lp)
{
#ifdef _WIN32
    HANDLE cdata;
    char *p, *localdata;
    int lencount;
    
    lencount = 0;
    localdata = NULL;
    if (OpenClipboard( GetConsoleWindow() )) {
        cdata = GetClipboardData(CF_TEXT);
        if (cdata) {
            p= (char *)GlobalLock(cdata);
            
            while (p[lencount]) {
                lencount++;
            }
            if (lencount) {
                localdata = (char *)malloc(lencount + 1);
                if (localdata) {
                    memcpy(localdata, p, lencount + 1);
                }
            }
            
            GlobalUnlock(cdata);
        }
        CloseClipboard();
    }
    
    if (localdata) {
        TextScreenBitmap *tempbitmap;
        int index, rle;
        
        rle = 0;
        index = 0;
        // check header
        // plain text or 'http://www.conwaylife.com/wiki/' s RLE,plain format
        while(localdata[index]) {
            if((localdata[index] == '#') || (localdata[index] == '!')) {
                while((localdata[index] != 0) && (localdata[index] != 0x0a)) {
                    index++;
                }
                if (localdata[index] == 0) break;
            } else if ((localdata[index] != 0x0d) && (localdata[index] != 0x0a) 
                        && (localdata[index] != ' ') && (localdata[index] != 0x09)) {
                if ((localdata[index] == 'x') || (localdata[index] == 'X')) {
                    rle = 1;
                }
                break;
            }
            index++;
        }
        
        // parse data
        TextScreen_ClearBitmap(lp->bitmap);
        if (rle) {
            ParseRLE(lp, localdata);
        } else {
            ParsePlainText(lp, localdata);
        }
        
        // loaded pattern move to center of bitmap
        CheckActive(lp);
        tempbitmap = TextScreen_DupBitmap(lp->bitmap);
        TextScreen_ClearBitmap(lp->bitmap);
        TextScreen_CopyBitmap(lp->bitmap, tempbitmap,
                (lp->bitmap->width - (lp->active.right - lp->active.left)) / 2 - lp->active.left,
                (lp->bitmap->height - (lp->active.bottom - lp->active.top)) / 2 - lp->active.top);
        TextScreen_FreeBitmap(tempbitmap);
        
        free(localdata);
        return 1;
    }
    return 0;
#else
    return 0;
#endif
}

// -------------------------------------------------------------
// print help text
// -------------------------------------------------------------
void PrintHelp(LifeParam *lp) {
    TextScreen_ClearScreen();
    TextScreen_SetCursorVisible(0);
    printf("\n\n");
    printf("  version info:\n");
    printf("  conways_game_of_life by Coffey 2015-2016\n");
    printf("  version %s  build: %s %s\n", VERSION_STR, __DATE__, __TIME__);
    printf("\n");
    printf("  [PageDown][s]slow  [PageUp][f]fast\n");
    printf("  [Enter][e]edit  [space]pause  [n]next generation\n");
    printf("  [arrow key]move view  [Home]reset view\n");
    printf("  [c]copy to clipboard(Windows Only)  [b]border/borderless\n");
    printf("  [u]edit alive/born rule  [y]edit field size\n");
    printf("  [r]restart  [h]help  [q]quit\n");
    printf("\n");
    printf("  Edit Mode:\n");
    printf("  [space]toggle  [arrow key]move cursor  [Enter][e]start\n");
    printf("  [x]clear  [c]copy to clipboard(Windows Only)\n");
    printf("  [v]paste from clipboard(Windows Only)\n");
    printf("  [r]read  [s]store  [p]load preset pattern\n");
    printf("  [u]edit alive/born rule  [y]edit field size\n");
    printf("  [h]help  [q]quit\n");
    printf("\n");
    printf("  hit any key\n");
    fflush(stdout);
    {  // wait any key
        int key = 0;
        while (!key) {
            key = TextScreen_GetKey();
            TextScreen_Wait(50);
            if (lp->quit) break;
        }
    }
    TextScreen_SetCursorVisible(0);
}

// -------------------------------------------------------------
// Set Rules
// -------------------------------------------------------------
void SetRule(LifeParam *lp)
{
    int i, quit;
    int curx, cury;
    int curoffsetx, curoffsety;
    
    TextScreen_ClearScreen();
    TextScreen_SetCursorVisible(1);
    
    TextScreen_SetCursorPos(0,2);
    printf("  Edit Alive/Born rules.\n");
    printf("\n");
    printf("                012345678\n");
    printf("  ------------------------\n");
    printf("  Stays alive : ");
    for (i = 0; i < 9; i++) {
        printf("%d", lp->rule_alive[i]);
    }
    printf("\n");
    printf("  Born        : ");
    for (i = 0; i < 9; i++) {
        printf("%d", lp->rule_born[i]);
    }
    printf("\n");
    printf("\n");
    printf("  [arrow key]select  [space]toggle  [d]set default  [Enter][u]exit\n");
    fflush(stdout);
    
    curx = 0;
    cury = 0;
    curoffsetx = 16;
    curoffsety = 6;
    TextScreen_SetCursorPos(curoffsetx + curx, curoffsety + cury);
    quit = 0;
    while (!quit && !lp->quit) {
        int  key;
        key = TextScreen_GetKey() & TSK_KEYMASK;
        if (key) {
            switch (key) {
                case 'u':  // exit edit
                case TSK_ENTER:
                    quit = 1;
                    break;
                case ' ':  // toggle rule
                    switch (cury) {
                        case 0:
                            lp->rule_alive[curx] = !(lp->rule_alive[curx]);
                            printf("%d", lp->rule_alive[curx]); 
                            TextScreen_SetCursorPos(curoffsetx + curx, curoffsety + cury);
                            break;
                        case 1:
                            lp->rule_born[curx] = !(lp->rule_born[curx]);
                            printf("%d", lp->rule_born[curx]); 
                            TextScreen_SetCursorPos(curoffsetx + curx, curoffsety + cury);
                            break;
                    }
                    break;
                case 'd':  // load default rules
                    SetDefaultRules(lp);
                    TextScreen_SetCursorPos(curoffsetx + 0, curoffsety + 0);
                    for (i = 0; i < 9; i++) {
                        printf("%d", lp->rule_alive[i]);
                    }
                    TextScreen_SetCursorPos(curoffsetx + 0, curoffsety + 1);
                    for (i = 0; i < 9; i++) {
                        printf("%d", lp->rule_born[i]);
                    }
                    TextScreen_SetCursorPos(curoffsetx + curx, curoffsety + cury);
                    break;
                case TSK_ARROW_UP:     // cursor up
                    cury--;
                    if (cury < 0) cury = 0;
                    break;
                case TSK_ARROW_DOWN:   // cursor down
                    cury++;
                    if (cury > 1) cury = 1;
                    break;
                case TSK_ARROW_LEFT:   // cursor left
                    curx--;
                    if (curx < 0) curx = 0;
                    break;
                case TSK_ARROW_RIGHT:  // cursor right
                    curx++;
                    if (curx > 8) curx = 8;
                    break;
            }
            TextScreen_SetCursorPos(curoffsetx + curx, curoffsety + cury);
        }
        TextScreen_Wait(10);
    }
    TextScreen_SetCursorVisible(0);
}

// -------------------------------------------------------------
// set board size
// -------------------------------------------------------------
void SetBoardSize(LifeParam *lp)
{
    int quit;
    int cury;
    int curoffsetx, curoffsety;
    int n[2];
    int width, height;
    
    TextScreen_ClearScreen();
    TextScreen_SetCursorVisible(1);
    
    n[0] = lp->bitmap->width;
    n[1] = lp->bitmap->height;
    
    TextScreen_SetCursorPos(0,2);
    printf("  Edit field size.\n");
    printf("\n");
    printf("  Width  : %5d\n", n[0]);
    printf("  Height : %5d\n", n[1]);
    printf("\n");
    printf("\n");
    printf("  [arrow Up/Down]select\n");
    printf("  [arrow Left/Right]change size(-1/+1)\n");
    printf("  [Page UP/Page Down]change size(-50/+50)\n");
    printf("  [d]set default  [Enter][y]exit\n");
    fflush(stdout);
    
    cury = 0;
    curoffsetx = 15;
    curoffsety = 4;
    TextScreen_SetCursorPos(curoffsetx, curoffsety + cury);
    quit = 0;
    while (!quit && !lp->quit) {
        int key;
        key = TextScreen_GetKey() & TSK_KEYMASK;
        if (key) {
            switch (key) {
                case 'y':  // exit edit
                case TSK_ENTER:
                    quit = 1;
                    break;
                case 'd':  // default size
                    n[0] = BOARD_SIZE_WIDTH;
                    n[1] = BOARD_SIZE_HEIGHT;
                    TextScreen_SetCursorVisible(0);
                    TextScreen_SetCursorPos(0, curoffsety);
                    printf("  Width  : %5d\n", n[0]);
                    printf("  Height : %5d\n", n[1]);
                    TextScreen_SetCursorVisible(1);
                    TextScreen_SetCursorPos(curoffsetx, curoffsety + cury);
                    break;
                case TSK_ARROW_UP:  // cursor up
                    cury--;
                    if (cury < 0) cury = 0;
                    break;
                case TSK_ARROW_DOWN:  // cursor down
                    cury++;
                    if (cury > 1) cury = 1;
                    break;
                case TSK_PAGEDOWN:    // page down
                case TSK_ARROW_LEFT:  // left
                case TSK_PAGEUP:      // page up
                case TSK_ARROW_RIGHT: // right
                    n[cury] += (key == TSK_ARROW_RIGHT) + (key == TSK_PAGEUP) * 50
                              -(key == TSK_ARROW_LEFT) - (key == TSK_PAGEDOWN) * 50;
                    if ((key == TSK_PAGEUP) || (key == TSK_PAGEDOWN)) {
                        n[cury] += (key == TSK_PAGEDOWN) * 49;
                        n[cury] = n[cury] - (n[cury] % 50);
                    }
                    if (n[cury] < 10) n[cury] = 10;
                    if (n[cury] > 10000) n[cury] = 10000;
                    TextScreen_SetCursorVisible(0);
                    TextScreen_SetCursorPos(0, curoffsety);
                    printf("  Width  : %5d\n", n[0]);
                    printf("  Height : %5d\n", n[1]);
                    TextScreen_SetCursorVisible(1);
                    break;
            }
            TextScreen_SetCursorPos(curoffsetx, curoffsety + cury);
        }
        TextScreen_Wait(10);
    }
    
    width  = n[0];
    height = n[1];
    if ((lp->bitmap->width != width) || (lp->bitmap->height != height)) {
        TextScreenBitmap *bitmap;
        
        // change view offset
        lp->offsetx = lp->offsetx - (lp->bitmap->width - lp->setting.width) / 2
                                  + (width - lp->setting.width) / 2;
        lp->offsety = lp->offsety - (lp->bitmap->height - lp->setting.height) / 2
                                  + (height - lp->setting.height) / 2;
        
        // resize internal bitmap (bitmap, storebitmap, restartbitmap)
        bitmap = TextScreen_CreateBitmap(width, height);
        TextScreen_CopyBitmap(bitmap, lp->bitmap,
                        (width - lp->bitmap->width) / 2,
                        (height - lp->bitmap->height) / 2);
        TextScreen_FreeBitmap(lp->bitmap);
        lp->bitmap = bitmap;
        
        bitmap = TextScreen_CreateBitmap(width, height);
        TextScreen_CopyBitmap(bitmap, lp->storebitmap,
                        (width - lp->storebitmap->width) / 2,
                        (height - lp->storebitmap->height) / 2);
        TextScreen_FreeBitmap(lp->storebitmap);
        lp->storebitmap = bitmap;
        
        bitmap = TextScreen_CreateBitmap(width, height);
        TextScreen_CopyBitmap(bitmap, lp->restartbitmap,
                        (width - lp->restartbitmap->width) / 2,
                        (height - lp->restartbitmap->height) / 2);
        TextScreen_FreeBitmap(lp->restartbitmap);
        lp->restartbitmap = bitmap;
        
        CheckActive(lp);
    }
    TextScreen_SetCursorVisible(0);
}

// -------------------------------------------------------------
// load preset pattern
// -------------------------------------------------------------
void LoadPreset(LifeParam *lp)
{
    // Preset Patterns ("title", "1st line", "2nd line", ... "")
    char *preset_pattern[] = {
        // F(R) pentomino
        "F(R) pentomino",
        ".##",
        "##",
        ".#",
        "",
        // Glider
        "Glider",
        ".#",
        "..#",
        "###",
        "",
        // Spaceship
        "Spaceship (lightweight, middleweight, heavyweight)",
        ".........................#..#",
        ".............................#",
        "...............#.........#...#",
        ".............#...#........####",
        "..................#",
        ".............#....#",
        "..##..........#####......#..#",
        "#....#.......................#",
        "......#..................#...#",
        "#.....#........#..........####",
        ".######......#...#",
        "..................#",
        ".............#....#......#..#",
        "..............#####..........#",
        ".........................#...#",
        "..........................####",
        "",
        // Spaceship Crash
        "Spaceship Crash",
        "..#.........................................#",
        "#...#.....................................#...#",
        ".....#...................................#",
        "#....#...................................#....#",
        ".#####...................................#####",
        ".",
        ".",
        ".",
        ".......#..#.........................#..#",
        "...........#.......................#",
        ".......#...#.......................#...#",
        "........####.......................####",
        "",
        // Diehard
        "Diehard",
        "......#",
        "##",
        ".#...###",
        "",
        // Acom
        "Acom",
        ".#",
        "...#",
        "##..###",
        "",
        // Gosper Glider Gun
        "Gosper Glider Gun",
        "........................#",
        "......................#.#",
        "............##......##",
        "...........#...#....##............##",
        "..........#.....#...##............##",
        "##........#...#.##....#.#",
        "##........#.....#.......#",
        "...........#...#",
        "............##",
        "",
        // Pulsar
        "Pulsar",
        "..###...###",
        ".",
        "#....#.#....#",
        "#....#.#....#",
        "#....#.#....#",
        "..###...###",
        ".",
        "..###...###",
        "#....#.#....#",
        "#....#.#....#",
        "#....#.#....#",
        ".",
        "..###...###",
        "",
        // Kok's Galaxy
        "Kok's Galaxy",
        "######.##",
        "######.##",
        ".......##",
        "##.....##",
        "##.....##",
        "##.....##",
        "##",
        "##.######",
        "##.######",
        "",
        // Pentadecathlon
        "Pentadecathlon",
        "..#....#",
        "##.####.##",
        "..#....#",
        "",
        // data termination string
        "" };
    int quit;
    int cury;
    int curoffsetx, curoffsety;
    int max;
    
    TextScreen_ClearScreen();
    TextScreen_SetCursorVisible(1);
    TextScreen_SetCursorPos(0,2);
    printf("  Load preset pattern.\n\n");
    {  // print pattern name.
        int str_n = 0, pattern_n = 1, title = 1;
        
        while (*preset_pattern[str_n]) {
            while (*preset_pattern[str_n]) {
                if (title) {
                    printf ("    %2d: %s\n", pattern_n, preset_pattern[str_n]);
                    title = 0;
                }
                str_n++;
            }
            title = 1;
            pattern_n++;
            str_n++;
        }
        max = pattern_n - 1;
    }
    printf("\n");
    printf("\n");
    printf("  [arrow Up/Down]select  [Enter][p]load  [Esc]exit without load\n");
    fflush(stdout);
    
    cury = 1;
    curoffsetx = 2;
    curoffsety = 3;
    TextScreen_SetCursorPos(curoffsetx, curoffsety + cury);
    printf("@");
    TextScreen_SetCursorPos(curoffsetx, curoffsety + cury);
    quit = 0;
    while (!quit && !lp->quit) {
        int key;
        key = TextScreen_GetKey() & TSK_KEYMASK;
        if (key) {
            switch (key) {
                case TSK_ESC:  // exit without load
                    cury = 0;
                    quit = 1;
                    break;
                case 'p':  // load current select and exit
                case TSK_ENTER:
                    quit = 1;
                    break;
                case TSK_ARROW_UP:  // cursor up
                    TextScreen_SetCursorPos(curoffsetx, curoffsety + cury);
                    printf(" ");
                    cury--;
                    if (cury < 1) cury = max;
                    TextScreen_SetCursorPos(curoffsetx, curoffsety + cury);
                    printf("@");
                    break;
                case TSK_ARROW_DOWN:  // cursor down
                    TextScreen_SetCursorPos(curoffsetx, curoffsety + cury);
                    printf(" ");
                    cury++;
                    if (cury > max) cury = 1;
                    TextScreen_SetCursorPos(curoffsetx, curoffsety + cury);
                    printf("@");
                    break;
            }
            TextScreen_SetCursorPos(curoffsetx, curoffsety + cury);
        }
        TextScreen_Wait(10);
    }
    if (cury) {  // selected
        int str_n = 0, pattern_n = 1, p = 0, title = 1;
        TextScreenBitmap *tempbitmap;
        int x, y;
        
        TextScreen_ClearBitmap(lp->bitmap);
        
        // load pattern
        while (*preset_pattern[str_n]) {
            while (*preset_pattern[str_n]) {
                if ((cury == pattern_n) && (!title)) {
                    TextScreen_DrawText(lp->bitmap, 0, p++, preset_pattern[str_n]);
                } else {
                    title = 0;
                }
                str_n++;
            }
            pattern_n++;
            if (pattern_n > cury)
                break;
            title = 1;
            str_n++;
        }
        
        // convert to internal format
        for (y = 0; y < lp->bitmap->height; y++) {
            for (x = 0; x < lp->bitmap->width; x++) {
                if (TextScreen_GetCell(lp->bitmap, x, y) == '#') {
                    TextScreen_PutCell(lp->bitmap, x, y, lp->aliveChar);
                } else {
                    TextScreen_ClearCell(lp->bitmap, x, y);
                }
            }
        }
        
        // move loaded pattern to center of bitmap
        CheckActive(lp);
        tempbitmap = TextScreen_DupBitmap(lp->bitmap);
        TextScreen_ClearBitmap(lp->bitmap);
        TextScreen_CopyBitmap(lp->bitmap, tempbitmap,
                (lp->bitmap->width - (lp->active.right - lp->active.left)) / 2 - lp->active.left,
                (lp->bitmap->height - (lp->active.bottom - lp->active.top)) / 2 - lp->active.top);
        TextScreen_FreeBitmap(tempbitmap);
    }
    TextScreen_SetCursorVisible(0);
}

// -------------------------------------------------------------
// check resize console (return 1=resized, 0=no resize)
// -------------------------------------------------------------
int ResizeConsole(LifeParam *lp)
{
    int width, height;
    int saveWidth, saveHeight;
    TextScreenSetting setting;
    
    TextScreen_GetConsoleSize(&width, &height);
    if((width == lp->consoleWidth) && (height == lp->consoleHeight)) {
        return 0;
    }
    
    saveWidth = lp->setting.width;
    saveHeight = lp->setting.height;
    
    lp->consoleWidth = width;
    lp->consoleHeight = height;
    TextScreen_ResizeScreen(0, 0);
    TextScreen_GetSetting(&setting);
    lp->setting = setting;
    
    // set new view offset
    lp->offsetx = lp->offsetx - (lp->bitmap->width - saveWidth) / 2
                              + (lp->bitmap->width - lp->setting.width) / 2;
    lp->offsety = lp->offsety - (lp->bitmap->height - saveHeight) / 2
                              + (lp->bitmap->height - lp->setting.height) / 2;
    
    TextScreen_ClearScreen();
    TextScreen_ShowBitmap(lp->bitmap, -lp->offsetx, -lp->offsety);
    
    return 1;
}

// -------------------------------------------------------------
// edit board
// -------------------------------------------------------------
void EditBoard(LifeParam *lp)
{
    int quit = 0;
    int curx, cury;
#ifdef _WIN32
    char *helptext = "  [space]toggle [Enter]start [x]clear [v]paste [r]read [s]store [h]help";
#else
    char *helptext = "  [space]toggle [Enter]start [x]clear [r]read [s]store [h]help         ";
#endif

#define EDITBOARD_CURSORCENTER() {  \
    curx = (lp->setting.width  + 1) / 2 + lp->setting.leftMargin;  \
    cury = (lp->setting.height + 1) / 2 + lp->setting.topMargin;   \
}
#define EDITBOARD_SHOWCENTER() {  \
    lp->offsetx = (lp->bitmap->width - lp->setting.width) / 2;     \
    lp->offsety = (lp->bitmap->height - lp->setting.height) / 2;   \
    TextScreen_ShowBitmap(lp->bitmap, -lp->offsetx, -lp->offsety); \
    EDITBOARD_CURSORCENTER();                                      \
}
#define  EDITBOARD_SHOWHELPLINE() {  \
    TextScreen_SetCursorPos(0, lp->setting.height + lp->setting.topMargin);  \
    printf("%s", helptext);                                                  \
    fflush(stdout);                                                          \
}

    TextScreen_ShowBitmap(lp->bitmap, -lp->offsetx, -lp->offsety);
    EDITBOARD_SHOWHELPLINE();
    EDITBOARD_CURSORCENTER()
    TextScreen_SetCursorPos(curx, cury);
    TextScreen_SetCursorVisible(1);
    
    while (!quit && !lp->quit) {
        int  key;
        key = TextScreen_GetKey() & TSK_KEYMASK;
        if (key) {
            switch (key) {
                case TSK_ENTER:  // (enter key) start life
                case 'e':  // exit edit mode
                    CheckActive(lp);
                    TextScreen_CopyBitmap(lp->restartbitmap, lp->bitmap, 0, 0);
                    lp->restartGenCount = lp->genCount;
                    quit = 1;
                    TextScreen_SetCursorPos(0, lp->setting.height + lp->setting.topMargin);
                    printf("                                                                          ");
                    break;
                // case TSK_ESC:  // esc key
                case 'q':  // quit program
                    quit = 1;
                    lp->quit = 1;
                    break;
                case 'c':  // copy to clipboard
                    SaveToClipboard(lp);
                    break;
                case 'u':  // change rule
                    SetRule(lp);
                    TextScreen_SetCursorVisible(1);
                    TextScreen_ShowBitmap(lp->bitmap, -lp->offsetx, -lp->offsety);
                    EDITBOARD_SHOWHELPLINE();
                    break;
                case 'y':  // edit board size
                    SetBoardSize(lp);
                    TextScreen_SetCursorVisible(1);
                    TextScreen_ShowBitmap(lp->bitmap, -lp->offsetx, -lp->offsety);
                    EDITBOARD_SHOWHELPLINE();
                    break;
                case 'v':  // load from clipboard (paste)
                    if (LoadFromClipboard(lp)) {
                        lp->genCount = 0;
                        EDITBOARD_SHOWCENTER();
                    }
                    break;
                case 'x':  // clear all
                    TextScreen_ClearBitmap(lp->bitmap);
                    lp->genCount = 0;
                    EDITBOARD_SHOWCENTER();
                    break;
                case 'r':  // read
                    TextScreen_CopyBitmap(lp->bitmap, lp->storebitmap, 0, 0);
                    lp->genCount = 0;
                    EDITBOARD_SHOWCENTER();
                    break;
                case 's':  // store
                    TextScreen_CopyBitmap(lp->storebitmap, lp->bitmap, 0, 0);
                    break;
                case 'p':  // load preset pattern
                    LoadPreset(lp);
                    TextScreen_SetCursorVisible(1);
                    lp->genCount = 0;
                    EDITBOARD_SHOWCENTER();
                    EDITBOARD_SHOWHELPLINE();
                    break;
                case ' ':  // toggle cell
                    {
                        int x, y;
                        x = lp->offsetx + curx - lp->setting.leftMargin;
                        y = lp->offsety + cury - lp->setting.topMargin;
                        if (TextScreen_GetCell(lp->bitmap, x, y) == lp->aliveChar) {
                            TextScreen_ClearCell(lp->bitmap, x, y);
                        } else {
                            TextScreen_PutCell(lp->bitmap, x, y, lp->aliveChar);
                        }
                        TextScreen_ShowBitmap(lp->bitmap, -lp->offsetx, -lp->offsety);
                    }
                    break;
                case 'h':  // show help
                    PrintHelp(lp);
                    TextScreen_SetCursorVisible(1);
                    TextScreen_ShowBitmap(lp->bitmap, -lp->offsetx, -lp->offsety);
                    EDITBOARD_SHOWHELPLINE();
                    break;
                case TSK_ARROW_UP:  // cursor up
                    cury--;
                    if (cury < lp->setting.topMargin) {
                        cury = lp->setting.topMargin;
                        lp->offsety--;
                        TextScreen_ShowBitmap(lp->bitmap, -lp->offsetx, -lp->offsety);
                    }
                    break;
                case TSK_ARROW_DOWN:  // cursor down
                    cury++;
                    if (cury >= lp->setting.height + lp->setting.topMargin) {
                        cury = lp->setting.height + lp->setting.topMargin - 1;
                        lp->offsety++;
                        TextScreen_ShowBitmap(lp->bitmap, -lp->offsetx, -lp->offsety);
                    }
                    break;
                case TSK_ARROW_LEFT:  // cursor left
                    curx--;
                    if (curx < lp->setting.leftMargin) {
                        curx = lp->setting.leftMargin;
                        lp->offsetx--;
                        TextScreen_ShowBitmap(lp->bitmap, -lp->offsetx, -lp->offsety);
                    }
                    break;
                case TSK_ARROW_RIGHT:  // cursor right
                    curx++;
                    if (curx >= lp->setting.width + lp->setting.leftMargin) {
                        curx = lp->setting.width + lp->setting.leftMargin - 1;
                        lp->offsetx++;
                        TextScreen_ShowBitmap(lp->bitmap, -lp->offsetx, -lp->offsety);
                    }
                    break;
                case TSK_PAGEUP:  // page up
                    break;
                case TSK_PAGEDOWN:  // page down
                    break;
                case TSK_HOME:  // home (view center)
                    EDITBOARD_SHOWCENTER();
                    break;
            }
            TextScreen_SetCursorPos(curx, cury);
        }
        if (ResizeConsole(lp)) {
            EDITBOARD_SHOWHELPLINE();
            EDITBOARD_CURSORCENTER()
            TextScreen_SetCursorPos(curx, cury);
        }
        TextScreen_Wait(10);
    }
    TextScreen_SetCursorVisible(0);
}

// -------------------------------------------------------------
// main loop keyboard check (return key code)
// -------------------------------------------------------------
int KeyboardCheck(LifeParam *lp)
{
    int key;
    
    key = TextScreen_GetKey() & TSK_KEYMASK;
    if (key) {
        switch (key) {
            // case TSK_ESC:  // esc key
            case 'q':  // quit
                printf("\n");
                lp->quit = 1;
                break;
            case 'f':  // fast: decrease wait
            case TSK_PAGEUP:  // page up(fast: decrease wait)
                lp->wait -= 1;
                if (lp->wait < 0) lp->wait = 0;
                break;
            case 's':  // slow: increase wait
            case TSK_PAGEDOWN:  // page down(slow: increase wait)
                lp->wait += 1;
                if (lp->wait > 100) lp->wait = 100;
                break;
            case ' ':  // pause
                lp->pause = (!(lp->pause));
                break;
            case 'n':  // next generation (use when pause)
                NextGeneration(lp);
                TextScreen_ShowBitmap(lp->bitmap, -lp->offsetx, -lp->offsety);
                break;
            case 'r':  // restart
                TextScreen_CopyBitmap(lp->bitmap, lp->restartbitmap, 0, 0);
                lp->genCount = lp->restartGenCount;
                CheckActive(lp);
                TextScreen_ShowBitmap(lp->bitmap, -lp->offsetx, -lp->offsety);
                break;
            case 'u':  // change rule
                SetRule(lp);
                TextScreen_ShowBitmap(lp->bitmap, -lp->offsetx, -lp->offsety);
                break;
            case 'y':  // edit board size
                SetBoardSize(lp);
                TextScreen_ShowBitmap(lp->bitmap, -lp->offsetx, -lp->offsety);
                break;
            case 'e':  // edit
            case 0x0d:
                EditBoard(lp);
                break;
            case 'h':  // show help
                PrintHelp(lp);
                TextScreen_ShowBitmap(lp->bitmap, -lp->offsetx, -lp->offsety);
                break;
            case 'c':  // copy (clipboard)
                SaveToClipboard(lp);
                break;
            case 'b':  // toggle borderless mode
                lp->borderless = (!(lp->borderless));
                break;
            case TSK_ARROW_UP:  // up
                lp->offsety -= 5;
                TextScreen_ShowBitmap(lp->bitmap, -lp->offsetx, -lp->offsety);
                break;
            case TSK_ARROW_DOWN:  // down
                lp->offsety += 5;
                TextScreen_ShowBitmap(lp->bitmap, -lp->offsetx, -lp->offsety);
                break;
            case TSK_ARROW_LEFT:  // left
                lp->offsetx -= 5;
                TextScreen_ShowBitmap(lp->bitmap, -lp->offsetx, -lp->offsety);
                break;
            case TSK_ARROW_RIGHT:  // right
                lp->offsetx += 5;
                TextScreen_ShowBitmap(lp->bitmap, -lp->offsetx, -lp->offsety);
                break;
            case TSK_HOME:  // home (view center)
                lp->offsetx = (lp->bitmap->width - lp->setting.width) / 2;
                lp->offsety = (lp->bitmap->height - lp->setting.height) / 2;
                TextScreen_ShowBitmap(lp->bitmap, -lp->offsetx, -lp->offsety);
                break;
        }
    }
    return key;
}

// -------------------------------------------------------------
// draw status line
// -------------------------------------------------------------
void DrawStatusLine(LifeParam *lp)
{
    TextScreen_SetCursorPos(0, lp->setting.height + lp->setting.topMargin);
    printf("  Gen %6d  View(%d,%d)  Wait %dms %s %s    [h]help            ", lp->genCount,
            lp->offsetx - (lp->bitmap->width - lp->setting.width) / 2,
            lp->offsety - (lp->bitmap->height - lp->setting.height) / 2,
            lp->wait * 10,
            lp->borderless ? "BL": "  ",
            lp->pause ? "Pause": "     ");
    fflush(stdout);
}

// -------------------------------------------------------------
// interrupt handler (press Ctrl+C)
// -------------------------------------------------------------
void SigintHandler(int sig, void *userdata)
{
    LifeParam *lp;
    
    lp = (LifeParam *)userdata;
    lp->quit = 1;
}

// -------------------------------------------------------------
// main entry
// -------------------------------------------------------------
int main(void)
{
    LifeParam lp;
    int waitCount;
    
    // initialize TextScreen
    TextScreen_Init(NULL);
    TextScreen_SetSpaceChar(BOARD_SPACE_CHAR);
    TextScreen_GetSetting(&lp.setting);
    // set interrupt hander (Ctrl+C)
    TextScreen_SetSigintHandler(SigintHandler, (void *)&lp);
    TextScreen_ClearScreen();
    TextScreen_SetCursorVisible(0);
    
    // initialize board
    InitLifeParam(&lp);
    
    // edit board
    EditBoard(&lp);
    
    waitCount = lp.wait;
    while (!lp.quit) {
        if ((waitCount <= 0) && (!lp.pause)) {
            NextGeneration(&lp);
            TextScreen_ShowBitmap(lp.bitmap, -lp.offsetx, -lp.offsety);
            waitCount = lp.wait;
        } else {
            TextScreen_Wait(10);
            waitCount--;
        }
        
        // console resize check
        ResizeConsole(&lp);
        // draw status line
        DrawStatusLine(&lp);
        // key check
        KeyboardCheck(&lp);
    }
    
    // move cursor to end of line, and 2 scrolls
    TextScreen_SetCursorPos(0, lp.consoleHeight - 1);
    printf("\n");
    TextScreen_Wait(50);
    printf("\n");
    fflush(stdout);
    // free board
    FreeLifeParam(&lp);
    // free TextScreen
    TextScreen_End();
    
    return 0;
}

