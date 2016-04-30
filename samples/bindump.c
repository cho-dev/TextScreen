/*****************************************
 bindump.c
 
 binary dump.
     (c)2016 Programming by Coffey
     Date: 20160429 - 20160430
 
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

// my translate table (control code -> '.')
static unsigned char gTranstable[256] = {
    0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e,
    0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e, 0x2e,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x2e,
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
    0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
    0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
    0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
    0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
    0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
    0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0x2e, 0x2e, 0x2e };

int read1b(FILE *fp, uint8_t *d)
{
    int  c;
    
    c = fgetc(fp);
    if (c == EOF) return -1;
    *d = (uint8_t)c;
    return 0;
}

int read_file(fname_t *filename, TextScreenBitmap *dumpmap, int64_t offset, int ascii7)
{
    FILE *fp;
    uint8_t  d8;
    int64_t  maxdata = MAX_VSIZE * 16;
    int64_t  start = offset * 16;
    int64_t  count = 0;
    char buf[64];
    
    fp = fopen_(filename, FILEMODE_READBINARY);
    if (fp == NULL ) {
        printf("\nFile could not open.(exist ?)\n");
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
        TextScreen_PutCell(dumpmap, (count % 16) + 60, count / 16, (ascii7 && (d8 >= 0x80)) ? 0x2e: d8);
        count++;
        if (count >= maxdata) break;
    }
    
    fclose(fp);
    return 0;
}

// make comma separated num string
void comma_separate_numstr(char *strbuf, int n, int64_t num)
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
    TextScreenBitmap   *bitmap, *dumpmap;
    TextScreenSetting  setting;
    int64_t y, prev_y, ofs, prev_ofs, filesize;
    int  consoleWidth, consoleHeight;
    int  key, redraw, reread, ascii7 = 1;
    int  ret;
    fname_t filename[1024];
    char    cfilename[1024];
    
    if (argc != 2) {
        printf("Binary Dump. (c)2016 by Coffey\n");
        printf("  Usage: %s <file name>\n", argv[0]);
        exit(0);
    }
    
    // get file name from argument
#ifdef _WIN32
    {  // for UNICODE(does not map to cp932) file name. eg. heart mark, etc...
        wchar_t  **argv_w;
        int      argc_w;
        
        argv_w = CommandLineToArgvW(GetCommandLineW(), &argc_w);
        if (!argv_w) {
            printf("Error: Cound not get file name string.\n");
            exit(-1);
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
            exit(-1);
        }
        fseek_(fp, 0, SEEK_END);
        filesize = ftell_(fp);
        fclose(fp);
    }
    
    // initialize TextScreen
    TextScreen_Init(0);
    // set my translate table
    TextScreen_GetSetting(&setting);
    setting.translate = (char *)gTranstable;
    TextScreen_SetSetting(&setting);
    // create bitmap, dumpmap
    bitmap  = TextScreen_CreateBitmap(0, 0);
    dumpmap = TextScreen_CreateBitmap(76, MAX_VSIZE);
    if (!bitmap || !dumpmap) {
        TextScreen_End();
        exit(-1);
    }
    
    // get current console size
    TextScreen_GetConsoleSize(&consoleWidth, &consoleHeight);
    
    // clear screen, hide cursor, set initial value
    TextScreen_ClearScreen();
    TextScreen_SetCursorVisible(0);
    y   = 0;
    ofs = 0;
    key = 0;
    redraw = 1;
    reread = 1;
    
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
                case TSK_HOME:
                    y = 0;
                    ofs = 0;
                    break;
                case TSK_END:
                    y = (filesize / 16) - ofs;
                    break;
                case '.':  // toggle 7bit/8bit ascii
                    ascii7 = !(ascii7);
                    reread = 1;
                    redraw = 1;
                    break;
                case 'r':  // reload
                    reread = 1;
                    redraw = 1;
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
                reread = 1;
                redraw = 1;
            }
            // view pointer changed ?
            if (prev_y != y) {
                redraw = 1;
            }
        }
        
        {  // console resize check
            int  width, height;
            
            TextScreen_GetConsoleSize(&width, &height);
            if ((width != consoleWidth) || (height != consoleHeight)) {
                TextScreen_ResizeScreen(0, 0);
                TextScreen_FreeBitmap(bitmap);
                bitmap = TextScreen_CreateBitmap(0, 0);
                if (!bitmap) {
                    TextScreen_End();
                    exit(-1);
                }
                TextScreen_ClearScreen();
                consoleWidth  = width;
                consoleHeight = height;
                redraw = 1;
            }
        }
        
        // read from file
        if (reread) {
            ret = read_file(filename, dumpmap, ofs, ascii7);
            if (ret) {
                TextScreen_End();
                exit(-1);
            }
            reread = 0;
        }
        // redraw screen
        if (redraw) {
            char buf[256];
            char nbuf1[32];
            char nbuf2[32];
            
            TextScreen_ClearBitmap(bitmap);
            TextScreen_DrawText(bitmap, 0, 0, headtext);
            TextScreen_DrawLine(bitmap, 0, 1, 75, 1, '-');
            TextScreen_CopyRect(bitmap, dumpmap, 0, 2, 0, y, 76, bitmap->height - 6, 0);
            TextScreen_DrawText(bitmap, 0, bitmap->height - 3, cfilename);
            comma_separate_numstr(nbuf1, sizeof(nbuf1), (y + ofs) * 16);
            comma_separate_numstr(nbuf2, sizeof(nbuf2), filesize);
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

