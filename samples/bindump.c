/*****************************************
 bindump.c
 
 binary dump.
     (c)2016 Programming by Coffey
     Date: 20160429
 
 build command
 (Windows) gcc bindump.c textscreen.c -lm -o bindump.exe
 (Linux  ) gcc bindump.c textscreen.c -lm -o bindump.out
 *****************************************/

// MSVC: ignore C4996 warning (fopen -> fopen_s etc...)
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#define _LARGEFILE_SOURCE
#define _FILE_OFFSET_BITS    64

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
#define  fseek_     _fseeki64
#define  ftell_     _ftelli64
#define  fopen_     _wfopen
typedef  wchar_t    fname_t;
#define  FILEMODE_READBINARY  L"rb"
#else  /* else _WIN32 */
#if _FILE_OFFSET_BITS == 64
#define  fseek_     fseeko
#define  ftell_     ftello
#else  /* else _FILE_OFFSET_BITS==64 */
#define  fseek_     fseek
#define  ftell_     ftell
#endif  /* end else _FILE_OFFSET_BITS==64 */
#define  fopen_     fopen
typedef  char       fname_t;
#define  FILEMODE_READBINARY  "rb"
#endif  /* end else _WIN32 */

#ifdef _MSC_VER
#define  snprintf   _snprintf
#define  snwprintf  _snwprintf
// for misssing <inttypes.h>
#define  PRId64     "I64d"
#define  PRIX64     "I64X"
#else
#include <inttypes.h>
#endif

#include "textscreen.h"

// MAX_VSIZE >= 512 and multiples of 256 (align 4096 bytes for read performance)
#define MAX_VSIZE  2048

int read1b(FILE *fp, uint8_t *d)
{
    int  c;
    
    c = fgetc(fp);
    if (c == EOF) return -1;
    *d = (uint8_t)c;
    return 0;
}

int read_file(fname_t *filename, TextScreenBitmap *dumpmap, int64_t offset)
{
    FILE *fp;
    uint8_t  d8;
    int64_t  maxdata = MAX_VSIZE * 16;
    int64_t  start = offset * 16;
    int64_t  count = 0;
    char buf[64];
    
    
    fp = fopen_(filename, FILEMODE_READBINARY);
    if (fp == NULL ) {
        printf("File could not open.(exist ?)\n");
        return -1;
    }
    
    if (fseek_(fp, start, SEEK_SET)) {
        fclose(fp);
        return 0;
    }
    
    TextScreen_ClearBitmap(dumpmap);
    while (!read1b(fp, &d8)) {
        if (count % 16 == 0) {
            snprintf(buf, sizeof(buf), "%08X", (unsigned int)(start + count));
            TextScreen_DrawText(dumpmap, 0, count / 16, buf);
            if (start + count >= 0x0000000100000000LL) {
                TextScreen_PutCell(dumpmap, 0, count / 16, '>');
            }
        }
        snprintf(buf, sizeof(buf), "%02X", (int)d8);
        TextScreen_DrawText(dumpmap, (count % 16) * 3 + 10, count / 16, buf);
        TextScreen_PutCell(dumpmap, (count % 16) + 60, count / 16, d8);
        count++;
        if (count >= maxdata) break;
    }
    
    fclose(fp);
    return 0;
}

// make comma separated num string
void comma_separate(char *strbuf, int n, int64_t num)
{
    int  i, j;
    
    snprintf(strbuf, n - 1, "%"PRId64, num);
    strbuf[n - 1] = 0;
    for (i = strlen(strbuf) % 3; i < (int)strlen(strbuf); i += 3) {
        if (i > (strbuf[0] == '-')) {
            for (j = strlen(strbuf); j >= i; j--) {
                if (j + 2 >= n) return;
                strbuf[j + 1] = strbuf[j];
            }
            strbuf[i] = ',';
            i++;
        }
    }
}

int main(int argc, char *argv[])
{
    char helptext[] = "[Arrow][PageUP/Down]scroll  [Home][End]Top/Bottom  [q][Esc]quit";
    char headtext[] = "Address | +0 +1 +2 +3 +4 +5 +6 +7 +8 +9 +A +B +C +D +E +F   0123456789ABCDEF";
    TextScreenBitmap *bitmap, *dumpmap;
    int64_t y, prev_y, ofs, prev_ofs, filesize;
    int  key, redraw;
    int  ret;
    fname_t filename[1024];
    char    cfilename[1024];
    
    if (argc != 2) {
        printf("Binary Dump. (c)2016 by Coffey\n");
        printf("  Usage: %s <file name>\n", argv[0]);
        return -1;
    }
    
#ifdef _WIN32
    {  // for UNICODE(does not map to cp932) file name. eg. heart mark, etc...
        wchar_t  **argv_w;
        int      argc_w;
        
        argv_w = CommandLineToArgvW(GetCommandLineW(), &argc_w);
        if (!argv_w) {
            printf("Error: Cound not get file name string.\n");
            return -1;
        }
        snwprintf(filename, sizeof(filename) - 1, L"%s", argv_w[1]);
        filename[MAX_PATH - 1] = 0;
        LocalFree(argv_w);
    }
#else
    snprintf(filename, sizeof(filename), "%s", argv[1]);
#endif
    snprintf(cfilename, sizeof(cfilename) - 1, "%s", argv[1]);
    cfilename[sizeof(cfilename) - 1] = 0;
    
    {  // get file size
        FILE *fp;
        fp = fopen_(filename, FILEMODE_READBINARY);
        if (fp == NULL ) {
            printf("File could not open (exist ?).\n");
            return -1;
        }
        fseek_(fp, 0, SEEK_END);
        filesize = ftell_(fp);
        fclose(fp);
    }
    
    // initialize TextScreen
    TextScreen_Init(0);
    bitmap  = TextScreen_CreateBitmap(0, 0);
    dumpmap = TextScreen_CreateBitmap(0, MAX_VSIZE);
    
    // read file
    ret = read_file(filename, dumpmap, 0);
    if (ret) {
        TextScreen_FreeBitmap(dumpmap);
        TextScreen_FreeBitmap(bitmap);
        TextScreen_End();
        return -1;
    }
    
    // clear screen, hide cursor, set initial value
    TextScreen_ClearScreen();
    TextScreen_SetCursorVisible(0);
    y   = 0;
    ofs = 0;
    key = 0;
    redraw = 1;
    
    // main loop
    while ((key != 'q') && (key != TSK_ESC)) {  // q or Esc key then quit
        prev_y = y;
        prev_ofs = ofs;
        if (key) {
            switch (key) {
                case 'k':
                case TSK_ARROW_UP:
                    y -= 1;
                    break;
                case ',':
                case TSK_ARROW_DOWN:
                    y += 1;
                    break;
                case 'j':
                case TSK_ARROW_LEFT:
                    y -= 0x10;
                    break;
                case 'm':
                case TSK_ARROW_RIGHT:
                    y += 0x10;
                    break;
                case 'h':
                case TSK_PAGEUP:
                    y -= 0x100;
                    break;
                case 'n':
                case TSK_PAGEDOWN:
                    y += 0x100;
                    break;
                case 'g':
                    y -= 0x1000;
                    break;
                case 'b':
                    y += 0x1000;
                    break;
                case 'f':
                    y -= 0x010000LL;
                    break;
                case 'v':
                    y += 0x10000LL;
                    break;
                case 'd':
                    y -= 0x100000LL;
                    break;
                case 'c':
                    y += 0x100000LL;
                    break;
                case 's':
                    y -= 0x1000000LL;
                    break;
                case 'x':
                    y += 0x1000000LL;
                    break;
                case 'a':
                    y -= 0x10000000LL;
                    break;
                case 'z':
                    y += 0x10000000LL;
                    break;
                case TSK_HOME:         // Home key
                    y = 0;
                    ofs = 0;
                    break;
                case TSK_END:          // End key
                    y = (filesize / 16) - ofs;
                    break;
            }
            // current pointer over the end of file
            if (y + ofs > (filesize / 16)) {
                y = (filesize / 16) - ofs;
            }
            // if current pointer is out of between 1/8-7/8 of dump bitmap, then recalc offset
            if ((y < MAX_VSIZE / 8) || (y > MAX_VSIZE * 7 / 8)) {
                int64_t  ofs1 = ofs;
                ofs = ((y + ofs) - (MAX_VSIZE / 2)) & 0xffffffffffffff00;
                if (ofs < 0) {
                    ofs = 0;
                }
                y = ofs1 - ofs + y;
                if (y < 0) {
                    y = 0;
                }
            }
            // if change offset, then read from file and redraw dumpmap
            if (prev_ofs != ofs) {
                ret = read_file(filename, dumpmap, ofs);
                if (ret) {
                    TextScreen_FreeBitmap(dumpmap);
                    TextScreen_FreeBitmap(bitmap);
                    TextScreen_End();
                    return -1;
                }
            }
            // view pointer changed ?
            if ((prev_y != y) || (prev_ofs != ofs)) {
                redraw = 1;
            }
        }
        // redraw screen
        if (redraw) {
            char buf[256];
            char nbuf1[32];
            char nbuf2[32];
            
            TextScreen_ClearBitmap(bitmap);
            TextScreen_DrawText(bitmap, 0, 0, headtext);
            TextScreen_DrawLine(bitmap, 0, 1, 75, 1, '-');
            TextScreen_CopyRect(bitmap, dumpmap, 0, 2, 0, y, bitmap->width, bitmap->height - 6, 0);
            TextScreen_DrawText(bitmap, 0, bitmap->height - 3, cfilename);
            comma_separate(nbuf1, sizeof(nbuf1), (y + ofs) * 16);
            comma_separate(nbuf2, sizeof(nbuf2), filesize);
            snprintf(buf, 255, "%"PRIX64"=%sbytes, MapOffset=%"PRId64", FileSize=%sbytes", 
                            (int64_t)((y + ofs) * 16), nbuf1, (int64_t)ofs, nbuf2);
            TextScreen_DrawText(bitmap, 0, bitmap->height - 2, buf);
            TextScreen_DrawText(bitmap, 0, bitmap->height - 1, helptext);
            TextScreen_ShowBitmap(bitmap, 0, 0);
            redraw = 0;
        }
        // sleep 10msec
        TextScreen_Wait(10);
        // check key input
        key = TextScreen_GetKey() & TSK_KEYMASK;
    }
    
    // move cursor to bottom line
    printf("\n");
    fflush(stdout);
    // free memory
    TextScreen_FreeBitmap(dumpmap);
    TextScreen_FreeBitmap(bitmap);
    // finish TextScreen
    TextScreen_SetCursorVisible(1);
    TextScreen_End();
    
    return 0;
}

