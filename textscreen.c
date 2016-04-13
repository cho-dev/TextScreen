/********************************
 textscreen.c
 
 TextScreen library. (C version)
     by Coffey (c)2015-2016
     
     VERSION 20160413
     
     Windows     : Win2K or later
     Non Windows : console support ANSI escape sequence
     
     Require : libm.so (-lm) for sqrt()
 
 TextScreen is free software, and under the MIT License.
 
*********************************/

// get more correct time in TextScreen_GetTickCount() for windows
// use timeGetTime()  (winmm.lib)
#define USE_WINMM 0

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#if USE_WINMM == 1
#include <mmsystem.h>
#endif
#else
// for nanosleep()
// #define _POSIX_C_SOURCE 199309L
// #define _POSIX_C_SOURCE 199506L
// for snprintf()
#define _POSIX_C_SOURCE 200112L
#include <time.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "textscreen.h"

// default settings
// default console size of most platform is usually 80x25 or 80x24
// this setting is best for it
#define SCREEN_DEFAULT_SIZE_X             76
#define SCREEN_DEFAULT_SIZE_Y             22
#define SCREEN_DEFAULT_TOP_MARGIN         1
#define SCREEN_DEFAULT_LEFT_MARGIN        2
#define SCREEN_DEFAULT_SAR                2.0
#define SCREEN_DEFAULT_SPACE_CHAR         ' '
#ifdef _WIN32
// #define SCREEN_DEFAULT_RENDERING_METHOD   TEXTSCREEN_RENDERING_METHOD_WINCONSOLE
#define SCREEN_DEFAULT_RENDERING_METHOD   TEXTSCREEN_RENDERING_METHOD_FAST
#else
#define SCREEN_DEFAULT_RENDERING_METHOD   TEXTSCREEN_RENDERING_METHOD_FAST
#endif

// ANSI escape code for terminal
#define P_CURSOR_UP()       {printf("\x1b[1A");fflush(stdout);}
#define P_CURSOR_DOWN()     {printf("\x1b[1B");fflush(stdout);}
#define P_CURSOR_FORWARD()  {printf("\x1b[1C");fflush(stdout);}
#define P_CURSOR_BACK()     {printf("\x1b[1D");fflush(stdout);}
#define P_ERASE_BELOW()     {printf("\x1b[0J");fflush(stdout);}
#define P_ERASE_ABOVE()     {printf("\x1b[1J");fflush(stdout);}
#define P_ERASE_ALL()       {printf("\x1b[2J");fflush(stdout);}
#define P_CLS()             {printf("\x1b[2J");fflush(stdout);}
#define P_RESET_STATE()     {printf("\x1b""c");fflush(stdout);}
#define P_CURSOR_POS(x,y)  {                                    \
    char strbuf[32];                                            \
    snprintf(strbuf, sizeof(strbuf), "\x1b[%d;%dH", y+1, x+1);  \
    printf("%s", strbuf);                                       \
    fflush(stdout);                                             \
}
#define P_CURSOR_SHOW()      {printf("\x1b[?25h");fflush(stdout);}
#define P_CURSOR_HIDE()      {printf("\x1b[?25l");fflush(stdout);}
#define P_SGR_RESET()        {printf("\x1b[0m");fflush(stdout);}

// key sequence table
struct KeySequence {
    int keycode;
    int num;
    unsigned char seq[8];
};

#ifdef _WIN32
static struct KeySequence gKeyScanWin [] = { // Windows
    { TSK_ARROW_LEFT,   2, { 0xe0, 0x4B, 0x00 }},
    { TSK_ARROW_RIGHT,  2, { 0xe0, 0x4D, 0x00 }},
    { TSK_ARROW_UP,     2, { 0xe0, 0x48, 0x00 }},
    { TSK_ARROW_DOWN,   2, { 0xe0, 0x50, 0x00 }},
    { TSK_INSERT,       2, { 0xe0, 0x52, 0x00 }},
    { TSK_DELETE,       2, { 0xe0, 0x53, 0x00 }},
    { TSK_HOME,         2, { 0xe0, 0x47, 0x00 }},
    { TSK_END,          2, { 0xe0, 0x4f, 0x00 }},
    { TSK_PAGEUP,       2, { 0xe0, 0x49, 0x00 }},
    { TSK_PAGEDOWN,     2, { 0xe0, 0x51, 0x00 }},
    
    { TSK_F1,           2, { 0x00, 0x3b, 0x00}},
    { TSK_F2,           2, { 0x00, 0x3c, 0x00}},
    { TSK_F3,           2, { 0x00, 0x3d, 0x00}},
    { TSK_F4,           2, { 0x00, 0x3e, 0x00}},
    { TSK_F5,           2, { 0x00, 0x3f, 0x00}},
    { TSK_F6,           2, { 0x00, 0x40, 0x00}},
    { TSK_F7,           2, { 0x00, 0x41, 0x00}},
    { TSK_F8,           2, { 0x00, 0x42, 0x00}},
    { TSK_F9,           2, { 0x00, 0x43, 0x00}},
    { TSK_F10,          2, { 0x00, 0x44, 0x00}},
    { TSK_F11,          2, { 0xe0, 0x85, 0x00}},
    { TSK_F12,          2, { 0xe0, 0x86, 0x00}},
    
    { TSK_ARROW_LEFT   | TSK_CTRL,    2, { 0xe0, 0x73, 0x00 }},
    { TSK_ARROW_RIGHT  | TSK_CTRL,    2, { 0xe0, 0x74, 0x00 }},
    { TSK_ARROW_UP     | TSK_CTRL,    2, { 0xe0, 0x8d, 0x00 }},
    { TSK_ARROW_DOWN   | TSK_CTRL,    2, { 0xe0, 0x91, 0x00 }},
    { TSK_ARROW_LEFT   | TSK_ALT,     2, { 0x00, 0x9b, 0x00 }},
    { TSK_ARROW_RIGHT  | TSK_ALT,     2, { 0x00, 0x9d, 0x00 }},
    { TSK_ARROW_UP     | TSK_ALT,     2, { 0x00, 0x98, 0x00 }},
    { TSK_ARROW_DOWN   | TSK_ALT,     2, { 0x00, 0xa0, 0x00 }},
    
    { TSK_INSERT       | TSK_CTRL,    2, { 0xe0, 0x92, 0x00 }},
    { TSK_DELETE       | TSK_CTRL,    2, { 0xe0, 0x93, 0x00 }},
    { TSK_HOME         | TSK_CTRL,    2, { 0xe0, 0x77, 0x00 }},
    { TSK_END          | TSK_CTRL,    2, { 0xe0, 0x75, 0x00 }},
    { TSK_PAGEUP       | TSK_CTRL,    2, { 0xe0, 0x86, 0x00 }},
    { TSK_PAGEDOWN     | TSK_CTRL,    2, { 0xe0, 0x76, 0x00 }},
    { TSK_INSERT       | TSK_ALT,     2, { 0x00, 0xa2, 0x00 }},
    { TSK_DELETE       | TSK_ALT,     2, { 0x00, 0xa3, 0x00 }},
    { TSK_HOME         | TSK_ALT,     2, { 0x00, 0x97, 0x00 }},
    { TSK_END          | TSK_ALT,     2, { 0x00, 0x9f, 0x00 }},
    { TSK_PAGEUP       | TSK_ALT,     2, { 0x00, 0x99, 0x00 }},
    { TSK_PAGEDOWN     | TSK_ALT,     2, { 0x00, 0xa1, 0x00 }},
    
    { TSK_ENTER        | TSK_CTRL,    1, { 0x0a, 0x00, 0x00 }},
    { TSK_BACKSPACE    | TSK_CTRL,    1, { 0x7f, 0x00, 0x00 }},
    
    // pad
    { TSK_ARROW_LEFT,   2, { 0x00, 0x4b, 0x00 }},
    { TSK_ARROW_RIGHT,  2, { 0x00, 0x4d, 0x00 }},
    { TSK_ARROW_UP,     2, { 0x00, 0x48, 0x00 }},
    { TSK_ARROW_DOWN,   2, { 0x00, 0x50, 0x00 }},
    { TSK_HOME,         2, { 0x00, 0x47, 0x00 }},
    { TSK_END,          2, { 0x00, 0x4f, 0x00 }},
    { TSK_PAGEUP,       2, { 0x00, 0x49, 0x00 }},
    { TSK_PAGEDOWN,     2, { 0x00, 0x51, 0x00 }},
    { TSK_ARROW_LEFT   | TSK_CTRL,    2, { 0x00, 0x73, 0x00 }},
    { TSK_ARROW_RIGHT  | TSK_CTRL,    2, { 0x00, 0x74, 0x00 }},
    { TSK_ARROW_UP     | TSK_CTRL,    2, { 0x00, 0x8d, 0x00 }},
    { TSK_ARROW_DOWN   | TSK_CTRL,    2, { 0x00, 0x91, 0x00 }},
    { TSK_HOME         | TSK_CTRL,    2, { 0x00, 0x77, 0x00 }},
    { TSK_END          | TSK_CTRL,    2, { 0x00, 0x75, 0x00 }},
    { TSK_PAGEUP       | TSK_CTRL,    2, { 0x00, 0x86, 0x00 }},
    { TSK_PAGEDOWN     | TSK_CTRL,    2, { 0x00, 0x76, 0x00 }},
    
    { TSK_F1           | TSK_SHIFT,   2, { 0x00, 0x54, 0x00}},
    { TSK_F2           | TSK_SHIFT,   2, { 0x00, 0x55, 0x00}},
    { TSK_F3           | TSK_SHIFT,   2, { 0x00, 0x56, 0x00}},
    { TSK_F4           | TSK_SHIFT,   2, { 0x00, 0x57, 0x00}},
    { TSK_F5           | TSK_SHIFT,   2, { 0x00, 0x58, 0x00}},
    { TSK_F6           | TSK_SHIFT,   2, { 0x00, 0x59, 0x00}},
    { TSK_F7           | TSK_SHIFT,   2, { 0x00, 0x5a, 0x00}},
    { TSK_F8           | TSK_SHIFT,   2, { 0x00, 0x5b, 0x00}},
    { TSK_F9           | TSK_SHIFT,   2, { 0x00, 0x5c, 0x00}},
    { TSK_F10          | TSK_SHIFT,   2, { 0x00, 0x5d, 0x00}},
    { TSK_F11          | TSK_SHIFT,   2, { 0xe0, 0x87, 0x00}},
    { TSK_F12          | TSK_SHIFT,   2, { 0xe0, 0x88, 0x00}},
    
    { TSK_F1           | TSK_CTRL,    2, { 0x00, 0x5e, 0x00}},
    { TSK_F2           | TSK_CTRL,    2, { 0x00, 0x5f, 0x00}},
    { TSK_F3           | TSK_CTRL,    2, { 0x00, 0x60, 0x00}},
    { TSK_F4           | TSK_CTRL,    2, { 0x00, 0x61, 0x00}},
    { TSK_F5           | TSK_CTRL,    2, { 0x00, 0x62, 0x00}},
    { TSK_F6           | TSK_CTRL,    2, { 0x00, 0x63, 0x00}},
    { TSK_F7           | TSK_CTRL,    2, { 0x00, 0x64, 0x00}},
    { TSK_F8           | TSK_CTRL,    2, { 0x00, 0x65, 0x00}},
    { TSK_F9           | TSK_CTRL,    2, { 0x00, 0x66, 0x00}},
    { TSK_F10          | TSK_CTRL,    2, { 0x00, 0x67, 0x00}},
    { TSK_F11          | TSK_CTRL,    2, { 0xe0, 0x89, 0x00}},
    { TSK_F12          | TSK_CTRL,    2, { 0xe0, 0x8a, 0x00}},
    
    { TSK_F1           | TSK_ALT,     2, { 0x00, 0x68, 0x00}},
    { TSK_F2           | TSK_ALT,     2, { 0x00, 0x69, 0x00}},
    { TSK_F3           | TSK_ALT,     2, { 0x00, 0x6a, 0x00}},
    { TSK_F4           | TSK_ALT,     2, { 0x00, 0x6b, 0x00}},
    { TSK_F5           | TSK_ALT,     2, { 0x00, 0x6c, 0x00}},
    { TSK_F6           | TSK_ALT,     2, { 0x00, 0x6d, 0x00}},
    { TSK_F7           | TSK_ALT,     2, { 0x00, 0x6e, 0x00}},
    { TSK_F8           | TSK_ALT,     2, { 0x00, 0x6f, 0x00}},
    { TSK_F9           | TSK_ALT,     2, { 0x00, 0x70, 0x00}},
    { TSK_F10          | TSK_ALT,     2, { 0x00, 0x71, 0x00}},
    { TSK_F11          | TSK_ALT,     2, { 0xe0, 0x8b, 0x00}},
    { TSK_F12          | TSK_ALT,     2, { 0xe0, 0x8c, 0x00}},
    
    { 0,                0, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }},
};
#else
static struct KeySequence gKeyEscSequence [] = {  // (ubuntu default)
    { TSK_ARROW_LEFT,   3, { 0x1b, 0x5b, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00 }},
    { TSK_ARROW_RIGHT,  3, { 0x1b, 0x5b, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00 }},
    { TSK_ARROW_UP,     3, { 0x1b, 0x5b, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00 }},
    { TSK_ARROW_DOWN,   3, { 0x1b, 0x5b, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00 }},
    { TSK_INSERT,       4, { 0x1b, 0x5b, 0x32, 0x7e, 0x00, 0x00, 0x00, 0x00 }},
    { TSK_DELETE,       4, { 0x1b, 0x5b, 0x33, 0x7e, 0x00, 0x00, 0x00, 0x00 }},
    { TSK_HOME,         3, { 0x1b, 0x4f, 0x48, 0x00, 0x00, 0x00, 0x00, 0x00 }},
    { TSK_END,          3, { 0x1b, 0x4f, 0x46, 0x00, 0x00, 0x00, 0x00, 0x00 }},
    { TSK_PAGEUP,       4, { 0x1b, 0x5b, 0x35, 0x7e, 0x00, 0x00, 0x00, 0x00 }},
    { TSK_PAGEDOWN,     4, { 0x1b, 0x5b, 0x36, 0x7e, 0x00, 0x00, 0x00, 0x00 }},
    
    { TSK_HOME,         4, { 0x1b, 0x5b, 0x31, 0x7e, 0x00, 0x00, 0x00, 0x00 }},
    { TSK_END,          4, { 0x1b, 0x5b, 0x34, 0x7e, 0x00, 0x00, 0x00, 0x00 }},
    
    { TSK_ARROW_LEFT   | TSK_SHIFT,   6, { 0x1b, 0x5b, 0x31, 0x3b, 0x32, 0x44, 0x00, 0x00 }},
    { TSK_ARROW_RIGHT  | TSK_SHIFT,   6, { 0x1b, 0x5b, 0x31, 0x3b, 0x32, 0x43, 0x00, 0x00 }},
    { TSK_ARROW_UP     | TSK_SHIFT,   6, { 0x1b, 0x5b, 0x31, 0x3b, 0x32, 0x41, 0x00, 0x00 }},
    { TSK_ARROW_DOWN   | TSK_SHIFT,   6, { 0x1b, 0x5b, 0x31, 0x3b, 0x32, 0x42, 0x00, 0x00 }},
    { TSK_ARROW_LEFT   | TSK_CTRL,    6, { 0x1b, 0x5b, 0x31, 0x3b, 0x35, 0x44, 0x00, 0x00 }},
    { TSK_ARROW_RIGHT  | TSK_CTRL,    6, { 0x1b, 0x5b, 0x31, 0x3b, 0x35, 0x43, 0x00, 0x00 }},
    { TSK_ARROW_UP     | TSK_CTRL,    6, { 0x1b, 0x5b, 0x31, 0x3b, 0x35, 0x41, 0x00, 0x00 }},
    { TSK_ARROW_DOWN   | TSK_CTRL,    6, { 0x1b, 0x5b, 0x31, 0x3b, 0x35, 0x42, 0x00, 0x00 }},
    { TSK_ARROW_LEFT   | TSK_ALT,     6, { 0x1b, 0x5b, 0x31, 0x3b, 0x33, 0x44, 0x00, 0x00 }},
    { TSK_ARROW_RIGHT  | TSK_ALT,     6, { 0x1b, 0x5b, 0x31, 0x3b, 0x33, 0x43, 0x00, 0x00 }},
    { TSK_ARROW_UP     | TSK_ALT,     6, { 0x1b, 0x5b, 0x31, 0x3b, 0x33, 0x41, 0x00, 0x00 }},
    { TSK_ARROW_DOWN   | TSK_ALT,     6, { 0x1b, 0x5b, 0x31, 0x3b, 0x33, 0x42, 0x00, 0x00 }},
    
    { TSK_PAGEUP       | TSK_CTRL,    6, { 0x1b, 0x5b, 0x35, 0x3b, 0x35, 0x7e, 0x00, 0x00 }},
    { TSK_PAGEDOWN     | TSK_CTRL,    6, { 0x1b, 0x5b, 0x36, 0x3b, 0x35, 0x7e, 0x00, 0x00 }},
    { TSK_PAGEUP       | TSK_ALT,     6, { 0x1b, 0x5b, 0x35, 0x3b, 0x33, 0x7e, 0x00, 0x00 }},
    { TSK_PAGEDOWN     | TSK_ALT,     6, { 0x1b, 0x5b, 0x36, 0x3b, 0x33, 0x7e, 0x00, 0x00 }},
    
    { TSK_INSERT       | TSK_ALT,     6, { 0x1b, 0x5b, 0x32, 0x3b, 0x33, 0x7e, 0x00, 0x00 }},
    { TSK_DELETE       | TSK_SHIFT,   6, { 0x1b, 0x5b, 0x33, 0x3b, 0x32, 0x7e, 0x00, 0x00 }},
    { TSK_DELETE       | TSK_CTRL,    6, { 0x1b, 0x5b, 0x33, 0x3b, 0x35, 0x7e, 0x00, 0x00 }},
    { TSK_DELETE       | TSK_ALT,     6, { 0x1b, 0x5b, 0x33, 0x3b, 0x33, 0x7e, 0x00, 0x00 }},
    
    { TSK_TAB          | TSK_SHIFT,   3, { 0x1b, 0x5b, 0x5a, 0x00, 0x00, 0x00, 0x00, 0x00 }},
    
    { 0,                0, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }},
};
#endif

// translate table
// <0x20(control code), 0x7f(del), 0xfd<  ===> space(' ')
static unsigned char gTranslateTable[256] = {
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x20,
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
    0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
    0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
    0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
    0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
    0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
    0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0x20, 0x20, 0x20 };

static TextScreenSetting gSetting = {0};

#ifdef _WIN32
#else
static int gSavedTermFlag = 0;
static struct termios gSavedTerm;
int TextScreen_SetNonBufferedTerm(void)
{
    struct termios term;
    int ret = 0;
    
    if (gSavedTermFlag) {
        ret = tcsetattr(STDIN_FILENO, TCSANOW, &gSavedTerm);
        gSavedTermFlag = 0;
    }
    ret = tcgetattr(STDIN_FILENO, &term);
    if (ret < 0) return ret;
    gSavedTerm = term;
    gSavedTermFlag = 1;
    term.c_lflag &= ~(ECHO | ICANON);
    term.c_cc[VMIN]  = 1;
    term.c_cc[VTIME] = 0;
    ret = tcsetattr(STDIN_FILENO, TCSANOW, &term);
    if (ret < 0) return ret;
    // note: set non-blocking mode at TextScreen_GetKey()
    
    return ret;
}
int TextScreen_RestoreTerm(void)
{
    int ret = 0;
    
    if (gSavedTermFlag) {
        ret = tcsetattr(0, TCSANOW, &gSavedTerm);
    }
    return ret;
}
#endif

// Windows prototype: BOOL HandlerRoutine(DWORD dwCtrlType)
int TextScreen_SIGINT_handler(int sig)
{
    int ret = 0;
    
#ifdef _WIN32
    // Windows sig value:
    // CTRL_C_EVENT 0 : CTRL_BREAK_EVENT 1 : CTRL_CLOSE_EVENT 2
    // CTRL_LOGOFF_EVENT 5 : CTRL_SHUTDOWN_EVENT 6
    sig = 2;   // set sig value to same as linux SIGINT
#endif
    if (gSetting.sigintHandler) {
        gSetting.sigintHandler(sig, gSetting.sigintHandlerUserData);
        ret = 1;
    } else {
        // sigintHnadler is not defined...
        TextScreen_End();
#ifdef _WIN32
        // Windows: use default ctrl handler (return=0)
#else
        // Lunux: force exit
        exit(sig);
#endif
    }
    return ret;
}

void TextScreen_SetSigintHandler(void (*handler)(int, void*), void *userdata)
{
    gSetting.sigintHandler = handler;
    gSetting.sigintHandlerUserData = userdata;
}

int TextScreen_Init(TextScreenSetting *usersetting)
{
    int ret = 0;
    
    if (!usersetting) {
        TextScreen_GetSettingDefault(&gSetting);
    } else {
        TextScreen_SetSetting(usersetting);
    }
#ifdef _WIN32
    ret = !SetConsoleCtrlHandler((PHANDLER_ROUTINE)TextScreen_SIGINT_handler, TRUE);
#else
    if (signal(SIGINT, (void *)&TextScreen_SIGINT_handler) == SIG_ERR)
        return -1;
    ret = TextScreen_SetNonBufferedTerm();
#endif
    return ret;
}

int TextScreen_End(void)
{
    int ret = 0;
#ifdef _WIN32
#else
    ret = TextScreen_RestoreTerm();
#endif
    TextScreen_SetCursorVisible(1);
    return ret;
}

int TextScreen_ClearScreen(void)
{
#ifdef _WIN32
    HANDLE stdh;
    COORD  coord;
    CONSOLE_SCREEN_BUFFER_INFO info;
    DWORD bufsize;
    DWORD len;
    
    stdh = GetStdHandle(STD_OUTPUT_HANDLE);
    if (!stdh) return -1;
    GetConsoleScreenBufferInfo(stdh, &info);
    
    // fill console buffer to space
    bufsize = info.dwSize.X * info.dwSize.Y;
    coord.X = 0;
    coord.Y = 0;
    FillConsoleOutputCharacter(stdh, ' ', bufsize, coord, &len);
    
    // set cursor position to (0,0)
    coord.X = 0;
    coord.Y = 0;
    SetConsoleCursorPosition(stdh ,coord);
#else
    // P_RESET_STATE();
    P_ERASE_ALL();
    P_CURSOR_POS(0, 0);
#endif
    return 0;
}

int TextScreen_ResizeScreen(int width, int height)
{
    if ((width < 0) || (height < 0)) return -1;
    
    if (!width || !height) {
        int w, h;
        TextScreen_GetConsoleSize(&w, &h);
        if (!width) {
            width = w - (SCREEN_DEFAULT_LEFT_MARGIN * 2);
            if (width < 1) return -1;
        }
        if (!height) {
            height = h - (SCREEN_DEFAULT_TOP_MARGIN * 2);
            if (height < 1) return -1;
        }
    }
    gSetting.width  = width;
    gSetting.height = height;
    return 0;
}

int TextScreen_GetConsoleSize(int *width, int *height)
{
#ifdef _WIN32
	HANDLE stdouth;
    CONSOLE_SCREEN_BUFFER_INFO info;
    int ret;
    
    stdouth = GetStdHandle(STD_OUTPUT_HANDLE);
    if (!stdouth) {
        *width  = 80;
        *height = 25;
        return -1;
    }
    ret = GetConsoleScreenBufferInfo(stdouth, &info);
    if (ret) {
        *width  = info.srWindow.Right-info.srWindow.Left + 1;
        *height = info.srWindow.Bottom-info.srWindow.Top + 1;
    } else {
        *width  = 80;
        *height = 25;
        return -1;
    }
    return 0;
#else
    struct winsize ws;
    if (ioctl(0, TIOCGWINSZ, &ws) != -1) {
        *width  = ws.ws_col;
        *height = ws.ws_row;
    } else {
        *width  = 80;
        *height = 25;
        return -1;
    }
    return 0;
#endif
}

int TextScreen_SetCursorPos(int x, int y)
{
#ifdef _WIN32
    HANDLE stdouth;
    COORD  coord;
    CONSOLE_SCREEN_BUFFER_INFO info;
    int    width, height;
    int    ret;
    
    stdouth = GetStdHandle(STD_OUTPUT_HANDLE);
    if (!stdouth) return -1;
    
    ret = GetConsoleScreenBufferInfo(stdouth, &info);
    if (ret) {
        width  = info.srWindow.Right-info.srWindow.Left + 1;
        height = info.srWindow.Bottom-info.srWindow.Top + 1;
    } else {
        return -1;
    }
    
    if (x < 0) x = 0;
    if (x >= width) x = width - 1;
    if (y < 0) y = 0;
    if (y >= height) y = height - 1;
    
    coord.X = (SHORT)x;
    coord.Y = (SHORT)y;
    SetConsoleCursorPosition(stdouth ,coord);
#else
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x > 32767) x = 32767;
    if (y > 32767) y = 32767;
    
    P_CURSOR_POS(x, y);
#endif
    return 0;
}

int TextScreen_SetCursorVisible(int visible)
{
#ifdef _WIN32
    HANDLE stdouth;
    CONSOLE_CURSOR_INFO cursorinfo;  // DWORD dwSize, BOOL bVisible
    
    stdouth = GetStdHandle(STD_OUTPUT_HANDLE);
    if (!stdouth) return -1;
    
    GetConsoleCursorInfo(stdouth, &cursorinfo);
    cursorinfo.bVisible = (visible != 0);
    SetConsoleCursorInfo(stdouth, &cursorinfo);
    return 0;
#else
    if (visible) {
        P_CURSOR_SHOW();
    } else {
        P_CURSOR_HIDE();
    }
    return 0;
#endif
}

void TextScreen_Wait(unsigned int ms)
{
#ifdef _WIN32
    Sleep(ms);
#else
    struct timespec t;
    
    t.tv_sec  = ms / 1000;
    t.tv_nsec = (ms % 1000) * 1000000L;
    nanosleep(&t, NULL);
#endif
}

unsigned int TextScreen_GetTickCount(void)
{
#ifdef _WIN32
    DWORD tick;
    
#if USE_WINMM == 1
    tick = timeGetTime();
#else
    tick = GetTickCount();
#endif
    return tick;
#else
    struct timeval tv;
    
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
#endif
}

void TextScreen_SetSpaceChar(char ch)
{
    gSetting.space = ch;
}

void TextScreen_SetRenderingMethod(int method)
{
    if ((method >= 0) && (method < TEXTSCREEN_RENDERING_METHOD_NB)) {
        gSetting.renderingMethod = method;
    }
}

void TextScreen_GetSetting(TextScreenSetting *setting)
{
    *setting = gSetting;
}

int TextScreen_SetSetting(TextScreenSetting *setting)
{
    if (!setting) return -1;
    
    gSetting = *setting;
    if (gSetting.sar < 0.1)
        gSetting.sar = 0.1;
    if (gSetting.sar > 10.0)
        gSetting.sar = 10.0;
    
    return 0;
}

void TextScreen_GetSettingDefault(TextScreenSetting *setting)
{
    int width, height;
    
    TextScreen_GetConsoleSize(&width, &height);
    
    setting->space      = SCREEN_DEFAULT_SPACE_CHAR;
    // setting->width      = SCREEN_DEFAULT_SIZE_X;
    // setting->height     = SCREEN_DEFAULT_SIZE_Y;
    setting->width      = width  - (SCREEN_DEFAULT_LEFT_MARGIN * 2);
    setting->height     = height - (SCREEN_DEFAULT_TOP_MARGIN * 2);
    setting->topMargin  = SCREEN_DEFAULT_TOP_MARGIN;
    setting->leftMargin = SCREEN_DEFAULT_LEFT_MARGIN;
    setting->sar        = SCREEN_DEFAULT_SAR;
    setting->renderingMethod = SCREEN_DEFAULT_RENDERING_METHOD;
    setting->sigintHandler = NULL;
    setting->sigintHandlerUserData = NULL;
    setting->translate  = (char *)gTranslateTable;
}

// #TODO: refactoring and improving code of TextScreen_GetKey()
int TextScreen_GetKey(void) {
#ifdef _WIN32
    int key = 0;
    int ch = 0;
    
    if (_kbhit()) {
        int j;
        unsigned char seq[8] = {0}
        ;
        ch = _getch();
        seq[0] = (unsigned char)ch;
        if (ch == 0xe0 || ch == 0x00) {
            ch = _getch();
            seq[1] = (unsigned char)ch;
            j = 0;
            while (gKeyScanWin[j].keycode) {
                if (gKeyScanWin[j].num == 2) {
                    if ((gKeyScanWin[j].seq[0] == seq[0]) && (gKeyScanWin[j].seq[1] == seq[1])) {
                        key = gKeyScanWin[j].keycode;
                        return key;
                    }
                }
                j++;
            }
            key = TSK_POSKEY + ch;
        } else {
            j = 0;
            while (gKeyScanWin[j].keycode) {
                if (gKeyScanWin[j].num == 1) {
                    if (gKeyScanWin[j].seq[0] == seq[0]) {
                        key = gKeyScanWin[j].keycode;
                        return key;
                    }
                }
                j++;
            }
            key = seq[0];
        }
    }
    return key;
#else
    int key = 0;
    int ch = 0;
    
    int nkey;
    int savefflag = 0;
    int ret;
    
    // set non-blocking mode
    savefflag = fcntl(STDIN_FILENO, F_GETFL);
    ret = fcntl(STDIN_FILENO, F_SETFL, savefflag | O_NONBLOCK);
    if (ret < 0) return 0;
    
    nkey = read(STDIN_FILENO, &ch, 1);
    if (nkey == 1) {
        key = ch;
        if (ch == 0x0a) {
            key = TSK_ENTER;
        }
        if (ch == 0x1b) {
            int i, j;
            unsigned char seq[8] = {0};
            
            seq[0] = ch;
            for (i = 2; i < 7; i++) {
                nkey = read(STDIN_FILENO, &ch, 1);
                if (nkey != 1) {
                    if (i == 2) {
                        fcntl(STDIN_FILENO, F_SETFL, savefflag);
                        return TSK_ESC;
                    } else {
                        fcntl(STDIN_FILENO, F_SETFL, savefflag);
                        return 0;
                    }
                }
                seq[i - 1] = ch;
                j = 0;
                while (gKeyEscSequence[j].keycode) {
                    if (gKeyEscSequence[j].num == i) {
                        if(!strcmp((char *)seq, (char *)gKeyEscSequence[j].seq)) {
                            key = gKeyEscSequence[j].keycode;
                            while (read(STDIN_FILENO, &ch, 1) == 1);
                            fcntl(STDIN_FILENO, F_SETFL, savefflag);
                            return key;
                        }
                    }
                    j++;
                }
            }
            while (read(STDIN_FILENO, &ch, 1) == 1);
            key = 0;
        }
    } else {
        key = 0;
    }
    // restore file mode
    fcntl(STDIN_FILENO, F_SETFL, savefflag);
    return key;
#endif
}


/********************************
 Bitmap Draw Tools
 ********************************/

void TextScreen_DrawFillCircle(TextScreenBitmap *bitmap, int x, int y, int r, char ch)
{
    int xd, yd, last_yd, last_xd;
    double m;
    
    if (!bitmap) return;
    if (r < 0)   return;
    
    xd = 0;
    yd = r;
    while(xd <= r * gSetting.sar) {
        last_yd = yd;
        m = (double)(r * r) - (double)(xd * xd) / (gSetting.sar * gSetting.sar);
        yd = (int)(sqrt(fabs(m)) + 0.5);
        if (last_yd - yd > 1) break;
        TextScreen_DrawLine(bitmap, x + xd, y + yd, x + xd, y - yd, ch);
        TextScreen_DrawLine(bitmap, x - xd, y + yd, x - xd, y - yd, ch);
        xd += 1;
    }
    xd = r;
    yd = 0;
    while(yd <= r) {
        last_xd = xd;
        m = (double)(r * r) - (double)(yd * yd);
        xd = (int)(sqrt(m) * gSetting.sar + 0.5);
        if (last_xd - xd > 1) break;
        TextScreen_DrawLine(bitmap, x + xd, y + yd, x - xd, y + yd, ch);
        TextScreen_DrawLine(bitmap, x + xd, y - yd, x - xd, y - yd, ch);
        yd += 1;
    }
}

void TextScreen_DrawCircle(TextScreenBitmap *bitmap, int x, int y, int r, char ch, int mode)
{
    int xd, yd, last_xd, last_yd;
    double m;
    
    if (!bitmap) return;
    if (r < 0)   return;
    
    if (mode == 1) {
        TextScreen_DrawFillCircle(bitmap, x, y, r, ch);
        return;
    }
    if (mode == 2)
        TextScreen_DrawFillCircle(bitmap, x, y, r, gSetting.space);
    
    xd = 0;
    yd = r;
    while(xd <= r * gSetting.sar) {
        last_yd = yd;
        m = (double)(r * r) - (double)(xd * xd) / (gSetting.sar * gSetting.sar);
        yd = (int)(sqrt(fabs(m)) + 0.5);
        if (last_yd - yd > 1) break;
        TextScreen_PutCell(bitmap, x + xd, y + yd, ch);
        TextScreen_PutCell(bitmap, x + xd, y - yd, ch);
        TextScreen_PutCell(bitmap, x - xd, y + yd, ch);
        TextScreen_PutCell(bitmap, x - xd, y - yd, ch);
        xd++;
    }
    xd = r;
    yd = 0;
    while(yd <= r) {
        last_xd = xd;
        m = (double)(r * r) - (double)(yd * yd);
        xd = (int)(sqrt(m) * gSetting.sar + 0.5);
        if (last_xd - xd > 1) break;
        TextScreen_PutCell(bitmap, x + xd, y + yd, ch);
        TextScreen_PutCell(bitmap, x + xd, y - yd, ch);
        TextScreen_PutCell(bitmap, x - xd, y + yd, ch);
        TextScreen_PutCell(bitmap, x - xd, y - yd, ch);
        yd++;
    }
}

void TextScreen_DrawFillRect(TextScreenBitmap *bitmap, int x, int y, int w, int h, char ch)
{
    int xmin, xmax, ymin, ymax;
    int xc, yc;
    
    if (!bitmap) return;
    xmin = x;
    xmax = x + w;
    ymin = y;
    ymax = y + h;
    
    if (xmin < 0) xmin = 0;
    if (xmax > bitmap->width) xmax = bitmap->width;
    if (ymin < 0) ymin = 0;
    if (ymax > bitmap->height) ymax = bitmap->height;
    
    for (yc = ymin; yc < ymax; yc++) {
        for (xc = xmin; xc < xmax; xc++) {
            TextScreen_PutCell(bitmap, xc, yc, ch);
        }
    }
}

void TextScreen_DrawRect(TextScreenBitmap *bitmap, int x, int y, int w, int h, char ch, int mode)
{
    int xc, yc;
    
    if (!bitmap) return;
    if (mode == 1) {
        TextScreen_DrawFillRect(bitmap, x, y, w, h, ch);
        return;
    }
    if (mode == 2) {
        TextScreen_DrawFillRect(bitmap, x+1, y+1, w-2, h-2, gSetting.space); 
    }
    
    for (xc = x; xc < x + w; xc++) {
        TextScreen_PutCell(bitmap, xc, y        , ch);
        TextScreen_PutCell(bitmap, xc, y + h - 1, ch);
    }
    for (yc = y; yc < y + h; yc++) {
        TextScreen_PutCell(bitmap, x        , yc, ch);
        TextScreen_PutCell(bitmap, x + w - 1, yc, ch);
    }
}

void TextScreen_DrawFillRectP(TextScreenBitmap *bitmap, int x, int y, int x1, int y1, char ch)
{
    TextScreen_DrawRectP(bitmap, x, y, x1, y1, ch, 1);
}

void TextScreen_DrawRectP(TextScreenBitmap *bitmap, int x, int y, int x1, int y1, char ch, int mode)
{
    int xs, ys, w, h;
    
    if (x <= x1) {
        xs = x;
        w  = x1 - x + 1;
    } else {
        xs = x1;
        w  = x - x1 + 1;
    }
    if (y <= y1) {
        ys = y;
        h  = y1 - y + 1;
    } else {
        ys = y1;
        h  = y - y1 + 1;
    }
    TextScreen_DrawRect(bitmap, xs, ys, w, h, ch, mode);
}

void TextScreen_DrawLine(TextScreenBitmap *bitmap, int x1, int y1, int x2, int y2, char ch)
{
    int xd, yd;
    int xda, yda;
    int xn, yn;
    int x, y;
    
    xd = x2 - x1;
    yd = y2 - y1;
    xda = (xd >= 0) ? xd : -xd;
    yda = (yd >= 0) ? yd : -yd;
    
    if (!bitmap) return;
    if (!xd) {
        if (yd >= 0) {
            for (y = y1; y <= y2; y++)
                TextScreen_PutCell(bitmap, x1, y, ch);
        } else {
            for (y = y1; y >= y2; y--)
                TextScreen_PutCell(bitmap, x1, y, ch);
        }
        return;
    }
    if (!yd) {
        if (xd >= 0) {
            for (x = x1; x <= x2; x++)
                TextScreen_PutCell(bitmap, x, y1, ch);
        } else {
            for (x = x1; x >= x2; x--)
                TextScreen_PutCell(bitmap, x, y1, ch);
        }
        return;
    }
    
    if (xda >= yda) {
       if ((xd >= 0) && (yd >= 0)) {
           for (x = x1; x <= x2; x++) {
               yn = y1 + ((yda*(x-x1)*256)/(xda) +128)/256;
               TextScreen_PutCell(bitmap, x, yn, ch);
           }
       }
       if ((xd >= 0) && (yd < 0)) {
           for (x = x1; x <= x2; x++) {
               yn = y1 - ((yda*(x-x1)*256)/(xda) +128)/256;
               TextScreen_PutCell(bitmap, x, yn, ch);
           }
       }
       if ((xd < 0) && (yd >= 0)) {
           for (x = x1; x >= x2; x--) {
               yn = y1 + ((yda*(x1-x)*256)/(xda) +128)/256;
               TextScreen_PutCell(bitmap, x, yn, ch);
           }
       }
       if ((xd < 0) && (yd < 0)) {
           for (x = x1; x >= x2; x--) {
               yn = y1 - ((yda*(x1-x)*256)/(xda) +128)/256;
               TextScreen_PutCell(bitmap, x, yn, ch);
           }
       }
    } else {
       if ((xd >= 0) && (yd >= 0)) {
           for (y = y1; y <= y2; y++) {
               xn = x1 + ((xda*(y-y1)*256)/(yda) +128)/256;
               TextScreen_PutCell(bitmap, xn, y, ch);
           }
       }
       if ((xd < 0) && (yd >= 0)) {
           for (y = y1; y <= y2; y++) {
               xn = x1 - ((xda*(y-y1)*256)/(yda) +128)/256;
               TextScreen_PutCell(bitmap, xn, y, ch);
           }
       }
       if ((xd >= 0) && (yd < 0)) {
           for (y = y1; y >= y2; y--) {
               xn = x1 + ((xda*(y1-y)*256)/(yda) +128)/256;
               TextScreen_PutCell(bitmap, xn, y, ch);
           }
       }
       if ((xd < 0) && (yd < 0)) {
           for (y = y1; y >= y2; y--) {
               xn = x1 - ((xda*(y1-y)*256)/(yda) +128)/256;
               TextScreen_PutCell(bitmap, xn, y, ch);
           }
       }
    }
}

void TextScreen_DrawText(TextScreenBitmap *bitmap, int x, int y, char *str)
{
    if (!bitmap) return;
    while (*str != '\0') {
        TextScreen_PutCell(bitmap, x, y, *str);
        x++;
        str++;
    }
}

char TextScreen_GetCell(TextScreenBitmap *bitmap, int x, int y)
{
    if (!bitmap) return 0;
    if ((x >= 0) && (x < bitmap->width) && (y >= 0) && (y < bitmap->height))
        return *(bitmap->data + y * bitmap->width + x);
    else
        return 0;
}

void TextScreen_PutCell(TextScreenBitmap *bitmap, int x, int y, char ch)
{
    if (!bitmap) return;
    if ((x >= 0) && (x < bitmap->width) && (y >= 0) && (y < bitmap->height))
        *(bitmap->data + y * bitmap->width + x) = ch;
}

void TextScreen_ClearCell(TextScreenBitmap *bitmap, int x, int y)
{
    if (!bitmap) return;
    if ((x >= 0) && (x < bitmap->width) && (y >= 0) && (y < bitmap->height))
        *(bitmap->data + y * bitmap->width + x) = gSetting.space;
}

void TextScreen_CopyRect(TextScreenBitmap *dstmap, TextScreenBitmap *srcmap, 
                         int dstx, int dsty, 
                         int srcx, int srcy, int srcw, int srch, 
                         int transparent)
{
    int  x, y;
    char ch;
    TextScreenBitmap *src, *dst, *tmp = NULL;
    
    if (!srcmap || !dstmap) return;
    
    src = srcmap;
    dst = dstmap;
    if (src == dst) {  // duplicate bitmap when source = destinate
        tmp = TextScreen_DupBitmap(src);
        src = tmp;
    }
    for (y = 0; y < srch; y++) {
        for (x = 0; x < srcw; x++) {
            ch = TextScreen_GetCell(src, srcx + x, srcy + y);
            if (ch != gSetting.space || !transparent)
                TextScreen_PutCell(dst, dstx + x, dsty + y, ch);
        }
    }
    if (tmp)
        TextScreen_FreeBitmap(tmp);
}

/********************************
 Bitmap Tools
 ********************************/

TextScreenBitmap *TextScreen_CreateBitmap(int width, int height)
{
    TextScreenBitmap *bitmap;
    char *data;
    
    if ((width < 0) || (width > TEXTSCREEN_MAXSIZE) || (height < 0) || (height > TEXTSCREEN_MAXSIZE)) {
        return NULL;
    }
    if (width == 0)
        width = gSetting.width;
    if (height == 0)
        height = gSetting.height;
    
    bitmap = (TextScreenBitmap *)malloc(sizeof(TextScreenBitmap));
    data   = (char *)malloc(width * height);
    if (!bitmap || !data) {
        if (bitmap) free(bitmap);
        if (data)   free(data);
        return NULL;
    }
    
    bitmap->width  = width;
    bitmap->height = height;
    bitmap->exdata = NULL;
    bitmap->data   = data;
    TextScreen_ClearBitmap(bitmap);
    
    return bitmap;
}

void TextScreen_FreeBitmap(TextScreenBitmap *bitmap)
{
    if (bitmap) {
        if (bitmap->data)
            free(bitmap->data);
        free(bitmap);
    }
}

void TextScreen_CopyBitmap(TextScreenBitmap *dstmap, TextScreenBitmap *srcmap, int dx, int dy)
{
    int x, y;
    
    if (!srcmap || !dstmap) return;
    
    for (y = 0; y < srcmap->height; y++) {
        for (x = 0; x < srcmap->width; x++) {
            TextScreen_PutCell(dstmap, x + dx, y + dy, TextScreen_GetCell(srcmap, x, y));
        }
    }
}

TextScreenBitmap *TextScreen_DupBitmap(TextScreenBitmap *bitmap)
{
    TextScreenBitmap *newmap;
    
    if (!bitmap) return NULL;
    
    newmap = TextScreen_CreateBitmap(bitmap->width, bitmap->height);
    if (newmap) {
        TextScreen_CopyBitmap(newmap, bitmap, 0, 0);
    }
    return newmap;
}

void TextScreen_OverlayBitmap(TextScreenBitmap *dstmap, TextScreenBitmap *srcmap, int dx, int dy)
{
    char ch;
    int x, y;
    
    if (!srcmap || !dstmap) return;
    
    for (y = 0; y < srcmap->height; y++) {
        for (x = 0; x < srcmap->width; x++) {
            ch = TextScreen_GetCell(srcmap, x, y);
            if (ch != gSetting.space)
                TextScreen_PutCell(dstmap, x + dx, y + dy, ch);
        }
    }
}

int TextScreen_CropBitmap(TextScreenBitmap *bitmap, int x, int y, int width, int height)
{
    char *data, *olddata;
    int  oldwidth, oldheight;
    int  xc, yc;
    char ch;
    
    if (!bitmap) return -1;
    if ((width < 1) || (width > TEXTSCREEN_MAXSIZE) || (height < 1) || (height > TEXTSCREEN_MAXSIZE)) {
        return -1;
    }
    data   = (char *)malloc(width * height);
    if (!data) return -1;
    
    oldwidth  = bitmap->width;
    oldheight = bitmap->height;
    olddata   = bitmap->data;
    
    bitmap->width  = width;
    bitmap->height = height;
    bitmap->data   = data;
    
    TextScreen_ClearBitmap(bitmap);
    
    for (yc = 0; yc < height; yc++) {
        for (xc = 0; xc < width; xc++) {
            if (((xc + x) >= 0) && ((yc + y) >= 0) && ((xc + x) < oldwidth) && ((yc + y) < oldheight)) {
                ch = *(olddata + ((yc + y) * oldwidth + (xc + x)));
                *(data + (yc * width + xc)) = ch;
            }
        }
    }
    
    free(olddata);
    
    return 0;
}

int TextScreen_ResizeBitmap(TextScreenBitmap *bitmap, int width, int height)
{
    char *data, *olddata;
    int  oldwidth, oldheight;
    int  xc, yc;
    char ch;
    
    if (!bitmap) return -1;
    if ((width < 1) || (width > TEXTSCREEN_MAXSIZE) || (height < 1) || (height > TEXTSCREEN_MAXSIZE)) {
        return -1;
    }
    data   = (char *)malloc(width * height);
    if (!data) return -1;
    
    oldwidth  = bitmap->width;
    oldheight = bitmap->height;
    olddata   = bitmap->data;
    
    bitmap->width  = width;
    bitmap->height = height;
    bitmap->data   = data;
    
    TextScreen_ClearBitmap(bitmap);
    // scaling with nearest neighbor
    for (yc = 0; yc < height; yc++) {
        for (xc = 0; xc < width; xc++) {
            ch = *(olddata + ((oldheight * yc / height) * oldwidth) + (oldwidth * xc / width));
            *(data + (yc * width + xc)) = ch;
        }
    }
    
    free(olddata);
    
    return 0;
}

int TextScreen_CompareBitmap(TextScreenBitmap *dstmap, TextScreenBitmap *srcmap, int dx, int dy)
{
    int  x, y;
    char chsrc, chdst;
    
    if (!srcmap || !dstmap) return -1;
    
    for (y = 0; y < srcmap->height; y++) {
        for (x = 0; x < srcmap->width; x++) {
            chsrc = TextScreen_GetCell(srcmap, x, y);
            chdst = TextScreen_GetCell(dstmap, x + dx, y + dy);
            if (chsrc > chdst) return 1;
            if (chsrc < chdst) return -1;
        }
    }
    return 0;
}

void TextScreen_ClearBitmap(TextScreenBitmap *bitmap)
{
    if (!bitmap) return;
    TextScreen_DrawFillRect(bitmap, 0, 0, bitmap->width, bitmap->height, gSetting.space);
}

int TextScreen_ShowBitmap(TextScreenBitmap *bitmap, int dx, int dy)
{
    char *buf;
    char ch;
    int  index;
    int  i, x, y;
    
    if (!gSetting.width || !gSetting.height)
        TextScreen_Init(NULL);
    
    if (!bitmap) return 0;
    buf = NULL;
    switch (gSetting.renderingMethod) {  // Create buffer
        case TEXTSCREEN_RENDERING_METHOD_WINCONSOLE:
        case TEXTSCREEN_RENDERING_METHOD_FAST:
            buf = (char *)malloc( (gSetting.topMargin * 2) + 
                    (gSetting.width+gSetting.leftMargin+2) * gSetting.height + 4 );
            if (!buf) return -1;
            break;
        case TEXTSCREEN_RENDERING_METHOD_NORMAL:
            buf = (char *)malloc(gSetting.width+gSetting.leftMargin+2);
            if (!buf) return -1;
            break;
        case TEXTSCREEN_RENDERING_METHOD_SLOW:
        default:
            break;
    }
    
#ifdef _WIN32
    {  // set cursor position to 0,0 (top-left corner)
        HANDLE stdh;
        COORD  coord;
        stdh = GetStdHandle(STD_OUTPUT_HANDLE);
        if (stdh) {
            coord.X = 0;
            coord.Y = 0;
            SetConsoleCursorPosition(stdh ,coord);
        } else {
            free(buf);
            return -1;
        }
    }
#else
    P_CURSOR_POS(0, 0);
#endif
    
    // these 'change sign' is historical reason.
    dx = -dx;
    dy = -dy;
    
    if (gSetting.renderingMethod == TEXTSCREEN_RENDERING_METHOD_NORMAL) {
        
        for (i = 0; i < gSetting.topMargin; i++)
            printf("\n");
        
        for (y = 0; y < gSetting.height; y++) {
            index = 0;
            for (i = 0; i < gSetting.leftMargin; i++) {
                buf[index++] = ' ';
            }
            for (x = 0; x < gSetting.width; x++) {
                ch = TextScreen_GetCell(bitmap, x + dx, y + dy);
                buf[index++] = gSetting.translate[(unsigned char)ch];
            }
            buf[index++] = 0;
            printf("%s\n", buf);
        }
    }
    
    if (gSetting.renderingMethod == TEXTSCREEN_RENDERING_METHOD_SLOW) {
        
        for (i = 0; i < gSetting.topMargin; i++)
            printf("\n");
        
        for (y = 0; y < gSetting.height; y++) {
            index = 0;
            for (i = 0; i < gSetting.leftMargin; i++) {
                fputc(' ', stdout);
            }
            for (x = 0; x < gSetting.width; x++) {
                ch = TextScreen_GetCell(bitmap, x + dx, y + dy);
                fputc(gSetting.translate[(unsigned char)ch], stdout);
            }
            printf("\n");
            TextScreen_Wait(0);
        }
    }
    
    if (gSetting.renderingMethod == TEXTSCREEN_RENDERING_METHOD_FAST) {
        index = 0;
        for (i = 0; i < gSetting.topMargin; i++) {
#ifdef _WIN32
            // buf[index++] = 0x0d;
            buf[index++] = 0x0a;
#else
            buf[index++] = 0x0a;
#endif
        }
        for (y = 0; y < gSetting.height; y++) {
            for (i = 0; i < gSetting.leftMargin; i++) {
                buf[index++] = ' ';
            }
            for (x = 0; x < gSetting.width; x++) {
                ch = TextScreen_GetCell(bitmap, x + dx, y + dy);
                buf[index++] = gSetting.translate[(unsigned char)ch];
            }
#ifdef _WIN32
            // buf[index++] = 0x0d;
            buf[index++] = 0x0a;
#else
            buf[index++] = 0x0a;
#endif
        }
        buf[index++] = 0;
        printf("%s", buf);
    }
    
    if (gSetting.renderingMethod == TEXTSCREEN_RENDERING_METHOD_WINCONSOLE) { // Windows only
#ifdef _WIN32
        HANDLE stdh;
        DWORD  wlen;
        
        index = 0;
        for (i = 0; i < gSetting.topMargin; i++) {
            // buf[index++] = 0x0d;
            buf[index++] = 0x0a;
        }
        for (y = 0; y < gSetting.height; y++) {
            for (i = 0; i < gSetting.leftMargin; i++) {
                buf[index++] = ' ';
            }
            for (x = 0; x < gSetting.width; x++) {
                ch = TextScreen_GetCell(bitmap, x + dx, y + dy);
                buf[index++] = gSetting.translate[(unsigned char)ch];
            }
            // buf[index++] = 0x0d;
            buf[index++] = 0x0a;
        }
        stdh = GetStdHandle(STD_OUTPUT_HANDLE);
        if (stdh) {
            WriteConsole(stdh, buf, index, &wlen, NULL);
        }
#endif
    }
    
    fflush(stdout);
    if (buf) free(buf);
    return 0;
}

