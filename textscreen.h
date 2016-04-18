/********************************
 textscreen.h
 
 TextScreen library. (C version)
     by Coffey (c)2015-2016
     
     Windows     : Win2K or later
     Non Windows : console support ANSI escape sequence
 
 TextScreen is free software, and under the MIT License.
 
********************************/

#ifndef TEXTSCREEN_TEXTSCREEN_H
#define TEXTSCREEN_TEXTSCREEN_H

#define TEXTSCREEN_TEXTSCREEN_VERSION 20160418

// max bitmap width and height
#define TEXTSCREEN_MAXSIZE 32768

// key code for TextScreen_GetKey()
#define TSK_BACKSPACE     0x00000008
#define TSK_TAB           0x00000009
#define TSK_ENTER         0x0000000D
#define TSK_ESC           0x0000001B
#define TSK_DEL           0x0000007F

#define TSK_POSKEY        0x00010000
#define TSK_FUNCKEY       0x00020000
#define TSK_SHIFT         0x00100000
#define TSK_CTRL          0x00200000
#define TSK_ALT           0x00400000

#define TSK_ARROW_LEFT    0x0001004B
#define TSK_ARROW_RIGHT   0x0001004D
#define TSK_ARROW_UP      0x00010048
#define TSK_ARROW_DOWN    0x00010050

#define TSK_INSERT        0x00010052
#define TSK_DELETE        0x00010053
#define TSK_HOME          0x00010047
#define TSK_END           0x0001004F
#define TSK_PAGEUP        0x00010049
#define TSK_PAGEDOWN      0x00010051

#define TSK_F1            0x00020041
#define TSK_F2            0x00020042
#define TSK_F3            0x00020043
#define TSK_F4            0x00020044
#define TSK_F5            0x00020045
#define TSK_F6            0x00020046
#define TSK_F7            0x00020047
#define TSK_F8            0x00020048
#define TSK_F9            0x00020049
#define TSK_F10           0x0002004a
#define TSK_F11           0x0002004b
#define TSK_F12           0x0002004c

#define TSK_KEYMASK       0x000FFFFF
#define TSK_MODKEYMASK    0x00F00000

// rendering method (output method)
enum TextScreenRenderingMethod {
    TEXTSCREEN_RENDERING_METHOD_FAST = 0,    // fast speed
    TEXTSCREEN_RENDERING_METHOD_NORMAL,      // normal speed. good quality
    TEXTSCREEN_RENDERING_METHOD_SLOW,        // output character 1 by 1 (use fputc) and sleep(0) by line
    TEXTSCREEN_RENDERING_METHOD_WINCONSOLE,  // use Windows console api (very fast, use WriteConsole())
    TEXTSCREEN_RENDERING_METHOD_NB           // number of method
};

typedef struct TextScreenSetting {
    // character code for space: use for ClearBitmap, except character for OverlayBitmap, ...
    char space;
    // screen width
    int  width;
    // screen height
    int  height;
    // screen top margin
    int  topMargin;
    // screen left margin
    int  leftMargin;
    // sample aspect ratio (0.1-10. use for DrawCircle)
    double  sar;
    // rendering method
    int  renderingMethod;
    // SIGINT callback handler
    void (*sigintHandler)(int sig, void *userdata);
    // SIGINT callback userdata
    void *sigintHandlerUserData;
    // translate table
    char *translate;
} TextScreenSetting;

typedef struct TextScreenBitmap {
    // bitmap width
    int width;
    // bitmap height
    int height;
    // extra user data  Note: This handle will not free automatically by TextScreen_FreeBitmap(). so manage by user.
    void *exdata;
    // bitmap data handle (size = width x height). Create by TextScreen_CreateBitmap()
    char *data;
} TextScreenBitmap;

// set SIGINT handler
void TextScreen_SetSigintHandler(void (*handler)(int, void*), void *userdata);

// initialize TextScreen library (NULL: use default setting)
int TextScreen_Init(TextScreenSetting *usersetting);

// Finish TextScreen library
int TextScreen_End(void);

// clear console
int TextScreen_ClearScreen(void);

// resize screen width, height (0,0):current console size
int TextScreen_ResizeScreen(int width, int height);

// get console size,  return  0: successful -1: error(could not get, width and height is set to default)
int TextScreen_GetConsoleSize(int *width, int *height);

// set cursor position (x, y),  (left, top) = (0, 0)
int TextScreen_SetCursorPos(int x, int y);

// set visible/hide cursor  0:hide  1:visible
int TextScreen_SetCursorVisible(int visible);

// wait for ms millisecond (call sleep)
void TextScreen_Wait(unsigned int ms);

// get tick count (msec)
unsigned int TextScreen_GetTickCount(void);

// set space character to ch
void TextScreen_SetSpaceChar(char ch);

// set rendering method
void TextScreen_SetRenderingMethod(int method);

// get copy of current settings
void TextScreen_GetSetting(TextScreenSetting *setting);

// set settings  0:successful  -1:error
int TextScreen_SetSetting(TextScreenSetting *setting);

// get copy of default settings
void TextScreen_GetSettingDefault(TextScreenSetting *setting);

// get key; return: key code
int TextScreen_GetKey(void);


// ******** bitmap draw tools ********
// bitmap handle=bitmap; center position=(x,y); radius=r; draw character=ch
void TextScreen_DrawFillCircle(TextScreenBitmap *bitmap, int x, int y, int r, char ch);

// bitmap handle=bitmap; center position=(x,y); radius=r; draw character=ch; mode(0:none,1:fill,2:fill space)
void TextScreen_DrawCircle(TextScreenBitmap *bitmap, int x, int y, int r, char ch, int mode);

// bitmap handle=bitmap; left=x; top=y; width=w; height=h; draw character=ch
void TextScreen_DrawFillRect(TextScreenBitmap *bitmap, int x, int y, int w, int h, char ch);

// bitmap handle=bitmap; left=x; top=y; width=w; height=h; draw character=ch; mode(0:none,1:fill,2:fill space)
void TextScreen_DrawRect(TextScreenBitmap *bitmap, int x, int y, int w, int h, char ch, int mode);

// bitmap handle=bitmap; left=x; top=y; right=x1; bottom=y1; draw character=ch
void TextScreen_DrawFillRectP(TextScreenBitmap *bitmap, int x, int y, int x1, int y1, char ch);

// bitmap handle=bitmap; left=x; top=y; right=x1; bottom=y1; draw character=ch; mode(0:none,1:fill,2:fill space)
void TextScreen_DrawRectP(TextScreenBitmap *bitmap, int x, int y, int x1, int y1, char ch, int mode);

// bitmap handle=bitmap; draw position(x1,y1) to (x2,y2); draw character=ch
void TextScreen_DrawLine(TextScreenBitmap *bitmap, int x1, int y1, int x2, int y2, char ch);

// bitmap handle=bitmap; draw position(x,y); text string=str
void TextScreen_DrawText(TextScreenBitmap *bitmap, int x, int y, char *str);

// bitmap handle=bitmap; get character at (x,y)
char TextScreen_GetCell(TextScreenBitmap *bitmap, int x, int y);

// bitmap handle=bitmap; put character ch to (x,y)
void TextScreen_PutCell(TextScreenBitmap *bitmap, int x, int y, char ch);

// bitmap handle=bitmap; clear (x,y)
void TextScreen_ClearCell(TextScreenBitmap *bitmap, int x, int y);

// copy rectangle: destination bitmap=dstmap; source bitmap=srcmap; dst-pos=(dstx, dsty); src-pos and size=(srcx, srcy, srcw, srch);
void TextScreen_CopyRect(TextScreenBitmap *dstmap, TextScreenBitmap *srcmap, 
                         int dstx, int dsty, int srcx, int srcy, int srcw, int srch, int transparent);


// ******** bitmap tools ********

// create bitmap handle (width x height),  if width=0 then width=gSetting.width, height=0 then height=gSetting.height
TextScreenBitmap *TextScreen_CreateBitmap(int width, int height);

// free bitmap handle
void TextScreen_FreeBitmap(TextScreenBitmap *bitmap);

// copy srcmap to dstmap(dx, dy) (not create new bitmap)
void TextScreen_CopyBitmap(TextScreenBitmap *dstmap, TextScreenBitmap *srcmap, int dx, int dy);

// duplicate bitmap handle (create new bitmap and copy)   Note: member 'exdata' will not duplicate cause unknown its size
TextScreenBitmap *TextScreen_DupBitmap(TextScreenBitmap *bitmap);

// copy srcmap to dstmap(dx, dy) except null character
void TextScreen_OverlayBitmap(TextScreenBitmap *dstmap, TextScreenBitmap *srcmap, int dx, int dy);

// crop bitmap; position(x, y)  size=(w x h),  return 0:successful  -1:failed
int TextScreen_CropBitmap(TextScreenBitmap *bitmap, int x, int y, int width, int height);

// resize bitmap; size=(w x h),  return 0:successful  -1:failed
int TextScreen_ResizeBitmap(TextScreenBitmap *bitmap, int width, int height);

// compare srcmap and dstmap(dx, dy),  return 0:same   1,-1:different
int TextScreen_CompareBitmap(TextScreenBitmap *dstmap, TextScreenBitmap *srcmap, int dx, int dy);

// clear bitmap (fill space character)
void TextScreen_ClearBitmap(TextScreenBitmap *bitmap);

// show bitmap to console. position of bitmap(0,0) = console(dx,dy),  return 0:successful  -1:error
int TextScreen_ShowBitmap(TextScreenBitmap *bitmap, int dx, int dy);

#endif

/* simple usage of this library ----------------------------------------
// build command (sample.c is this sample)
// (Windows) gcc sample.c textscreen.c -lm -o sample.exe
// (Linux  ) gcc sample.c textscreen.c -lm -o sample.out

#include "textscreen.h"

int main(void)
{
    TextScreenBitmap   *bitmap, *sprite;      // bitmap pointer
    int  x, y, xd, yd, key;
    char *helptext =  "Press [q] or [Esc] to exit";
    
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
*/

