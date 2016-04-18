/**********************************************************
 conway's game of life
     programming by Coffey   20151030
               last modified 20160418
     for Windows, Linux(ubuntu)
 require:
 textscree.c, textscreen.h (version >= 20160406)
 
 build:(-std=c99)
 gcc life.c textscreen.c -Wall -lm -o life.exe (Windows)
 gcc life.c textscreen.c -Wall -lm -o life.out (Linux)
 Require 'xsel' command to use clipboard for Linux.
 **********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#ifdef _WIN32
// include windows.h for Clipboard API
#include <windows.h>
#endif

// default board size
#define BOARD_SIZE_WIDTH    500
#define BOARD_SIZE_HEIGHT   500
// max board size
#define BOARD_SIZE_MAX      10000
// version string
#define VERSION_STR         "1.52"

#define BOARD_SPACE_CHAR    '.'
#define BOARD_SURVIVE_CHAR  '#'

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
    char surviveChar;
    Rect active;
    int  borderless;
    int  rule_survive[9];
    int  rule_born[9];
    char name[256];
    char author[256];
    char comment[512];
    int  patternWidth;
    int  patternHeight;
} LifeParam;

// -------------------------------------------------------------
// fix for MS(msvcrt) snprintf
// -------------------------------------------------------------
int snprintf_a(char *buffer, size_t count, const char *format, ...)
{
    int ret;
    va_list ap;
    
    va_start(ap, format);
#ifdef _WIN32
#ifdef _MSC_VER
    ret = _vsnprintf_s(buffer, count, _TRUNCATE, format, ap);
#else
    ret = vsnprintf(buffer, count - 1, format, ap);
    buffer[count - 1] = 0;
#endif
#else
    ret = vsnprintf(buffer, count, format, ap);
#endif
    va_end(ap);
    return ret;
}

// -------------------------------------------------------------
// set default rules to LifeParam
// -------------------------------------------------------------
void SetDefaultRules(LifeParam *lp)
{
    // set default rule (B3/S23)
    lp->rule_survive[0] = 0;
    lp->rule_survive[1] = 0;
    lp->rule_survive[2] = 1;
    lp->rule_survive[3] = 1;
    lp->rule_survive[4] = 0;
    lp->rule_survive[5] = 0;
    lp->rule_survive[6] = 0;
    lp->rule_survive[7] = 0;
    lp->rule_survive[8] = 0;
    
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
// Clear pattern info
// -------------------------------------------------------------
void ClearPatternInfo(LifeParam *lp)
{
    lp->name[0] = 0;
    lp->author[0] = 0;
    lp->comment[0] = 0;
    lp->patternWidth = 0;
    lp->patternHeight = 0;
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
    lp->surviveChar = BOARD_SURVIVE_CHAR;
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
    ClearPatternInfo(lp);
    
    // set default rule
    SetDefaultRules(lp);
    
    // initial pattern (F-pentomino)
    TextScreen_PutCell(lp->bitmap, width / 2    , height / 2 - 1, lp->surviveChar);
    TextScreen_PutCell(lp->bitmap, width / 2 + 1, height / 2 - 1, lp->surviveChar);
    TextScreen_PutCell(lp->bitmap, width / 2 - 1, height / 2    , lp->surviveChar);
    TextScreen_PutCell(lp->bitmap, width / 2    , height / 2    , lp->surviveChar);
    TextScreen_PutCell(lp->bitmap, width / 2    , height / 2 + 1, lp->surviveChar);
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
// detect active region (rectangle)
// return number of survive cell
// -------------------------------------------------------------
int CheckActive(LifeParam *lp)
{
    int x, y;
    int count = 0;
    
    lp->active.top = lp->bitmap->height - 1;
    lp->active.bottom = 0;
    lp->active.left = lp->bitmap->width - 1;
    lp->active.right = 0;
    for (y = 0; y < lp->bitmap->height; y++) {
        for (x = 0; x < lp->bitmap->width; x++) {
            if (TextScreen_GetCell(lp->bitmap, x, y) == lp->surviveChar) {
                if (x < lp->active.left) lp->active.left = x;
                if (x > lp->active.right) lp->active.right = x;
                if (y < lp->active.top) lp->active.top = y;
                if (y > lp->active.bottom) lp->active.bottom = y;
                count++;
            }
        }
    }
    if (lp->active.left > lp->active.right) {
        lp->active.left = lp->active.right;
        lp->active.top = lp->active.bottom;
        count = 0;
    }
    return count;
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
                neighbor = (TextScreen_GetCell(bitmapSrc, (x - 1 + w) % w, (y - 1 + h) % h) == lp->surviveChar)
                         + (TextScreen_GetCell(bitmapSrc, (x + 0 + w) % w, (y - 1 + h) % h) == lp->surviveChar)
                         + (TextScreen_GetCell(bitmapSrc, (x + 1 + w) % w, (y - 1 + h) % h) == lp->surviveChar)
                         + (TextScreen_GetCell(bitmapSrc, (x - 1 + w) % w, (y + 0 + h) % h) == lp->surviveChar)
                         + (TextScreen_GetCell(bitmapSrc, (x + 1 + w) % w, (y + 0 + h) % h) == lp->surviveChar)
                         + (TextScreen_GetCell(bitmapSrc, (x - 1 + w) % w, (y + 1 + h) % h) == lp->surviveChar)
                         + (TextScreen_GetCell(bitmapSrc, (x + 0 + w) % w, (y + 1 + h) % h) == lp->surviveChar)
                         + (TextScreen_GetCell(bitmapSrc, (x + 1 + w) % w, (y + 1 + h) % h) == lp->surviveChar);
            } else {
                neighbor = (TextScreen_GetCell(bitmapSrc, x - 1, y - 1) == lp->surviveChar)
                         + (TextScreen_GetCell(bitmapSrc, x + 0, y - 1) == lp->surviveChar)
                         + (TextScreen_GetCell(bitmapSrc, x + 1, y - 1) == lp->surviveChar)
                         + (TextScreen_GetCell(bitmapSrc, x - 1, y + 0) == lp->surviveChar)
                         + (TextScreen_GetCell(bitmapSrc, x + 1, y + 0) == lp->surviveChar)
                         + (TextScreen_GetCell(bitmapSrc, x - 1, y + 1) == lp->surviveChar)
                         + (TextScreen_GetCell(bitmapSrc, x + 0, y + 1) == lp->surviveChar)
                         + (TextScreen_GetCell(bitmapSrc, x + 1, y + 1) == lp->surviveChar);
            }
            if (TextScreen_GetCell(bitmapSrc, x, y) == lp->surviveChar) {
                if (lp->rule_survive[neighbor]) {
                    TextScreen_PutCell(bitmapDst, x, y, lp->surviveChar);
                }
            } else {
                if (lp->rule_born[neighbor]) {
                    TextScreen_PutCell(bitmapDst, x, y, lp->surviveChar);
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

// =============================================================
// Text Parse Utility
// =============================================================
// -------------------------------------------------------------
// To lowercase
// -------------------------------------------------------------
void ToLowercase(char *str)
{
    while (*str) {
        if ((*str >= 'A') && (*str <= 'Z')) {
            *str = *str + ('a' - 'A');
        }
        str++;
    }
}

// -------------------------------------------------------------
// To uppercase
// -------------------------------------------------------------
void ToUppercase(char *str)
{
    while (*str) {
        if ((*str >= 'a') && (*str <= 'z')) {
            *str = *str - ('a' - 'A');
        }
        str++;
    }
}

// -------------------------------------------------------------
// Triming (remove head and tail spaces, tabs)
// -------------------------------------------------------------
void TrimSpace(char *str)
{
    char *src = str;
    char *dst = str;
    
    while ((*src == ' ') || (*src == '\t'))
        src++;
    while (*src)
        *dst++ = *src++;
    *dst = 0;
    while ((*dst == 0) || (*dst == ' ') || (*dst == '\t')) {
        *dst = 0;
        if (dst == str)
            break;
        dst--;
    }
}

// -------------------------------------------------------------
// Skip white space (' ' or tabs)
// -------------------------------------------------------------
void SkipWhiteSpace(char **p)
{
    while ((**p == ' ') || (**p == '\t'))
        (*p)++;
}

// -------------------------------------------------------------
// Skip to next line
// -------------------------------------------------------------
void SkipToNextLine(char **p)
{
    while ((**p) && (**p != 0x0a))
        (*p)++;
    if (**p == 0x0a)
        (*p)++;
}

// -------------------------------------------------------------
// end of line ?
// -------------------------------------------------------------
int isEndOfLine(char **p)
{
    return ((**p == 0) || (**p == 0x0a) || (**p == 0x0d));
}

// -------------------------------------------------------------
// end of data ?
// -------------------------------------------------------------
int isEndOfData(char **p)
{
    return (**p == 0);
}

// -------------------------------------------------------------
// Extract Key and Value (key<eq>value<delim>)
// -------------------------------------------------------------
void ExtractKeyValue(char *key, int keylen, char *val, int vallen, char **p, char eq, char delim)
{
    int kp = 0, vp = 0;
    
    key[0] = 0;
    val[0] = 0;
    
    while(!isEndOfLine(p) && (**p != delim) && (**p != eq)) {
        if (kp < keylen - 1) {
            key[kp++] = **p;
        }
        (*p)++;
    }
    key[kp++] = 0;
    if (isEndOfLine(p)) {
        return;
    }
    if (**p == delim) {
        (*p)++;
        return;
    }
    if (**p == eq) {
        (*p)++;
    }
    while(!isEndOfLine(p) && (**p != delim)) {
        if (vp < vallen - 1) {
            val[vp++] = **p;
        }
        (*p)++;
    }
    val[vp++] = 0;
    if (**p == delim) {
        (*p)++;
    }
    TrimSpace(key);
    TrimSpace(val);
}

// -------------------------------------------------------------
// Extract line (current pos to end of line)
// -------------------------------------------------------------
void ExtractLine(char *buf, int buflen, char **p)
{
    int  bufp = 0;
    
    while(!isEndOfLine(p)) {
        if (bufp < buflen - 1) {
            buf[bufp++] = **p;
        }
        (*p)++;
    }
    buf[bufp++] = 0;
    TrimSpace(buf);
}
// =============================================================
// end of Parse Utility
// =============================================================

// -------------------------------------------------------------
// Set rules
// string format:
//     Bbb/Sss (eg. B3/S23  B36/S23)
//     Sss/Bbb (eg. S23/B3  S23/B36)
//     sss/bbb (eg. 23/3    23/36)
//     BbbSss, B=bb/S=ss also readable.
// -------------------------------------------------------------
void SetRules(LifeParam *lp, const char *srule)
{
    int  i;
    int  prefixExist;
    char prefix;
    char rule[64];
    
    // make string copy
    snprintf_a(rule, sizeof(rule), srule);
    
    // trim and lowercase
    TrimSpace(rule);
    ToLowercase(rule);
    if (!rule[0]) {  // if no rules then set default
        SetDefaultRules(lp);
        return;
    }
    
    // reset rule
    for (i = 0; i < 9; i++) {
        lp->rule_survive[i] = 0;
        lp->rule_born[i]    = 0;
    }
    
    prefixExist =  ((rule[0] == 'b') || (rule[0] == 's'));
    
    i = 0;
    prefix = 's';
    while (rule[i]) {
        if ((rule[i] == 'b') || (rule[i] == 's'))
            prefix = rule[i];
        if ((rule[i] == '/') && (!prefixExist))
            prefix = 'b';
        if ((rule[i] >= '0') && (rule[i] <= '8')) {
            if (prefix == 'b') {
                lp->rule_born[rule[i] - '0'] = 1;
            }
            if (prefix == 's') {
                lp->rule_survive[rule[i] - '0'] = 1;
            }
        }
        i++;
    }
}

// -------------------------------------------------------------
// Parse RLE header
// See http://www.conwaylife.com/wiki/RLE
// -------------------------------------------------------------
char *ParseRLEHeader(LifeParam *lp, char *rletext)
{
    char *p;
    char buf[256], vbuf[256];
    char tmp[512];
    char op;
    
    ClearPatternInfo(lp);
    SetDefaultRules(lp);
    
    p = rletext;
    SkipWhiteSpace(&p);
    while(*p == '#') {
        p++;
        op = *p++;
        switch (op) {
            case 'C':  // comment line
            case 'c':  // same as 'C' (deprecated)
                ExtractLine(buf, sizeof(buf), &p);
                if (lp->comment[0]) {
                    snprintf_a(tmp, sizeof(tmp), "%s", lp->comment);
                    snprintf_a(lp->comment, sizeof(lp->comment), "%s\n  %s", tmp, buf);
                } else {
                    snprintf_a(lp->comment, sizeof(lp->comment), "  %s", buf);
                }
                break;
            case 'N':  // pattern name 
                ExtractLine(lp->name, sizeof(lp->name), &p);
                break;
            case 'O':  // Author and Create date
                ExtractLine(lp->author, sizeof(lp->author), &p);
                break;
            case 'P':  // represent coordinate (top left)
                ExtractLine(buf, sizeof(buf), &p);
                break;
            case 'R':  // pattern coodinate (top left)
                ExtractLine(buf, sizeof(buf), &p);
                break;
            case 'r':  // rules for the pattern 23/3 B3/S23 B36/S23 (B=born, S=survive)
                ExtractLine(buf, sizeof(buf), &p);
                SetRules(lp, buf);
                break;
            default:
                break;
        }
        SkipToNextLine(&p);
        SkipWhiteSpace(&p);
    }
    
    // if no header line (x,y,rule) then return
    if (((*p >= '0') && (*p <= '9')) || (*p == 'b') || (*p == 'o') || (*p == '$'))
        return p;
    
    // read header line (defined x,y,rule)
    while(!isEndOfLine(&p)) {
        ExtractKeyValue(buf, sizeof(buf), vbuf, sizeof(vbuf), &p, '=', ',');
        ToLowercase(buf);
        if (!strcmp(buf, "x")) {
            lp->patternWidth = atoi(vbuf);
        }
        if (!strcmp(buf, "y")) {
            lp->patternHeight = atoi(vbuf);
        }
        if (!strcmp(buf, "rule")) {
            SetRules(lp, vbuf);
        }
    }
    SkipToNextLine(&p);
    return p;
}

// -------------------------------------------------------------
// Parse RLE format (ignore header)
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
            case 0x0a:
            case 0x0d:
                break;
            case 0x09:
            case ' ':
                snum = 0;
                break;
            case '0':  // num of run length
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
            case 'b':  // born
                if (snum == 0) snum = 1;
                x += snum;
                snum = 0;
                break;
            case 'O':
            case 'o':  // survive
                if (snum == 0) snum = 1;
                for (i = 0; i < snum; i++) {
                    TextScreen_PutCell(lp->bitmap, x, y, lp->surviveChar);
                    x++;
                }
                snum = 0;
                break;
            case '$':  // line feed
                if (snum == 0) snum = 1;
                x = 0;
                y += snum;
                if (y >= lp->bitmap->height) quit = 1;
                snum = 0;
                break;
            case '!':  // end of data
                quit = 1;
                break;
            case '#':  // comment line
            case 'X':  // header x = **
            case 'x':  // header x = **
            default:
                while (rletext[index] && rletext[index] != 0x0a) {
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
// Parse plain text header
// See http://www.conwaylife.com/wiki/Plaintext
// -------------------------------------------------------------
char *ParsePlainTextHeader(LifeParam *lp, char *plaintext)
{
    char *p;
    char buf[256], vbuf[256];
    char key[256];
    char tmp[512];
    
    ClearPatternInfo(lp);
    SetDefaultRules(lp);
    
    p = plaintext;
    SkipWhiteSpace(&p);
    while(*p == '!') {
        p++;
        ExtractKeyValue(buf, sizeof(buf), vbuf, sizeof(vbuf), &p, ':', ',');
        snprintf_a(key, sizeof(tmp), "%s", buf);
        ToLowercase(key);
        if (!strcmp(key, "name")) {
            snprintf_a(lp->name, sizeof(lp->name), "%s", vbuf);
        } else if (!strcmp(key, "author")) {
            snprintf_a(lp->author, sizeof(lp->author), "%s", vbuf);
        } else if (!strcmp(key, "rule")) {
            SetRules(lp, vbuf);
        } else {
            if (lp->comment[0]) {
                snprintf_a(tmp, sizeof(tmp), "%s", lp->comment);
                snprintf_a(lp->comment, sizeof(lp->comment), "%s\n  %s", tmp, buf);
            } else {
                snprintf_a(lp->comment, sizeof(lp->comment), "  %s", buf);
            }
        }
        SkipToNextLine(&p);
        SkipWhiteSpace(&p);
    }
    return p;
}

// -------------------------------------------------------------
// Parse plain text
// This code can read Plaintext format of http://www.conwaylife.com/wiki/Main_Page
// and clipboard data made by this program.
// About Plaintext format, see See http://www.conwaylife.com/wiki/Plaintext
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
                if (y >= lp->bitmap->height) {
                    quit = 1;
                }
                break;
            case 0x0d:
                break;
            case 'O':   // for http://www.conwaylife.com/wiki/Plaintext
            case BOARD_SURVIVE_CHAR:
                TextScreen_PutCell(lp->bitmap, x, y, lp->surviveChar);
                x++;
                break;
            case '!':
                while (plaintext[index] && (plaintext[index] != 0x0a)) {
                    index++;
                }
                if (plaintext[index] == 0x0a) {
                    x = 0;
                    y++;
                    if (y >= lp->bitmap->height) {
                        quit = 1;
                    }
                }
                if (plaintext[index] == 0) {
                    quit = 1;
                }
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
// Clipboard (Copy)
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
                    p[index++] = (cell == lp->surviveChar) ? BOARD_SURVIVE_CHAR : BOARD_SPACE_CHAR;
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
    FILE *fp;
    char cell;
    int  x, y, ret = 0;
    int  ch;
    
    // is xsel command exist ?
    fp = popen("which xsel", "r");
    if (fp == NULL) {
        return -1;
    }
    ch = fgetc(fp);
    pclose(fp);
    if ((ch == EOF) || (ch != '/')) {
        return -1;
    }
    
    // use xsel command to put data to clipboard
    fp = popen("xsel --clipboard --input", "w");
    if (fp == NULL) {
        return -1;
    }
    for (y = 0; y < lp->bitmap->height; y++) {
        for (x = 0; x < lp->bitmap->width; x++) {
            cell = TextScreen_GetCell(lp->bitmap, x, y);
            cell = (cell == lp->surviveChar) ? BOARD_SURVIVE_CHAR : BOARD_SPACE_CHAR;
            if (fputc(cell, fp) == EOF) {
                ret = -1;
                break;
            }
        }
        if (ret) break;
        if (fputc(0x0a, fp) == EOF) {
            ret = -1;
            break;
        }
    }
    pclose(fp);
    
    return ret;
#endif
}

// -------------------------------------------------------------
// Clipboard (Paste)
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
#else
    char buf[1024];
    char *p, *localdata;
    FILE *fp;
    int  memsize = 8192;
    int  expandsize = 8192;
    int  pos = 0;
    int  ch;
    
    // is xsel command exist ?
    fp = popen("which xsel", "r");
    if (fp == NULL) {
        return -1;
    }
    ch = fgetc(fp);
    pclose(fp);
    if ((ch == EOF) || (ch != '/')) {
        return -1;
    }
    
    // get initial buffer
    localdata = (char *)malloc(memsize);
    if (!localdata) {
        return -1;
    }
    
    // use xsel command to get from clipboard
    fp = popen("xsel --clipboard --output", "r");
    if (fp == NULL) {
        free(localdata);
        return -1;
    }
    while(1) {
        p = fgets(buf, sizeof(buf), fp);
        if (p == NULL) break;
        while (*p) {
            localdata[pos++] = *p;
            p++;
            // expand buffer
            if (pos >= memsize) {
                char *tmp;
                tmp = (char *)malloc(memsize + expandsize);
                if (!tmp) {
                    free(localdata);
                    pclose(fp);
                    return -1;
                }
                memcpy(tmp, localdata, pos);
                free(localdata);
                localdata = tmp;
                memsize += expandsize;
            }
        }
        if (feof(fp))  break;
    }
    localdata[pos] = 0;
    pclose(fp);
    
    // clipboard was empty
    if (!localdata[0]) {
        free(localdata);
        localdata = NULL;
    }
#endif
    
    if (localdata) {
        TextScreenBitmap *tempbitmap;
        int index, rle;
        char *pdata;
        
        rle = 0;
        index = 0;
        // detect Plaintext or RLE format
        while(localdata[index]) {
            while((localdata[index] == ' ') || (localdata[index] == '\t')) {
                index++;
            }
            if((localdata[index] == '#') || (localdata[index] == '!')) {
                while(localdata[index] && (localdata[index] != 0x0a)) {
                    index++;
                }
                if (localdata[index] == 0) break;
            } else if ((localdata[index] == '$') || (localdata[index] == 'x') ||
                       (localdata[index] == 'y') || (localdata[index] == 'B') ||
                       (localdata[index] == 'b') || (localdata[index] == 'o')) {
                // use char $,x,y,B,b,o in the data, then RLE
                rle = 1;
                break;
            }
            index++;
        }
        
        // parse data
        TextScreen_ClearBitmap(lp->bitmap);
        pdata = localdata;
        if (rle) {
            pdata = ParseRLEHeader(lp, pdata);
            ParseRLE(lp, pdata);
        } else {
            pdata = ParsePlainTextHeader(lp, pdata);
            ParsePlainText(lp, pdata);
        }
        
        // loaded pattern move to center of bitmap
        if (CheckActive(lp)) {
            if (!lp->patternWidth && !lp->patternHeight) {  // if pattern size was not found from header
                lp->patternWidth  = lp->active.right - lp->active.left + 1;
                lp->patternHeight = lp->active.bottom - lp->active.top + 1;
            }
        }
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
}

// -------------------------------------------------------------
// print help text
// -------------------------------------------------------------
void PrintHelp(LifeParam *lp) {
    TextScreen_ClearScreen();
    TextScreen_SetCursorVisible(0);
    printf("\n\n");
    printf("  Version Info.\n");
    printf("  conways_game_of_life by Coffey 2015-2016\n");
    printf("  version %s  build: %s %s\n", VERSION_STR, __DATE__, __TIME__);
    printf("\n");
    printf("  [PageDown][s]slow  [PageUp][f]fast\n");
    printf("  [Enter][e]edit  [space]pause  [n]next generation\n");
    printf("  [arrow key]move view  [Home]reset view\n");
    printf("  [c]copy to clipboard  [b]border/borderless\n");
    printf("  [u]edit survive/born rule  [y]edit field size\n");
    printf("  [r]restart  [h]help  [i]info  [q]quit\n");
    printf("\n");
    printf("  Edit Mode:\n");
    printf("  [space]toggle  [arrow key]move cursor  [Enter][e]start\n");
    printf("  [x]clear  [c]copy to clipboard  [v]paste from clipboard\n");
    printf("  [r]read  [s]store  [p]load preset pattern\n");
    printf("  [u]edit survive/born rule  [y]edit field size\n");
    printf("  [h]help  [i]info  [q]quit\n");
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
// print board info
// -------------------------------------------------------------
void PrintInfo(LifeParam *lp) {
    TextScreen_ClearScreen();
    TextScreen_SetCursorVisible(0);
    printf("\n\n");
    printf("  Pattern Info.\n");
    printf("\n");
    printf("  Pattern Name : %s\n", lp->name);
    printf("  Author       : %s\n", lp->author);
    if ((!lp->patternWidth) && (!lp->patternHeight)) {
        printf("  Initial Size : n/a\n");
    } else {
        printf("  Initial Size : %d x %d\n", lp->patternWidth, lp->patternHeight);
    }
    printf("  Rule(Survive): %d%d%d%d%d%d%d%d%d\n", lp->rule_survive[0], lp->rule_survive[1],
                                                    lp->rule_survive[2], lp->rule_survive[3],
                                                    lp->rule_survive[4], lp->rule_survive[5],
                                                    lp->rule_survive[6], lp->rule_survive[7],
                                                    lp->rule_survive[8]);
    printf("  Rule(Born)   : %d%d%d%d%d%d%d%d%d\n", lp->rule_born[0], lp->rule_born[1],
                                                    lp->rule_born[2], lp->rule_born[3],
                                                    lp->rule_born[4], lp->rule_born[5],
                                                    lp->rule_born[6], lp->rule_born[7],
                                                    lp->rule_born[8]);
    printf("\n");
    printf("  Comment      :\n%s\n", lp->comment);
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
    TextScreen_ClearScreen();
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
    printf("  Edit Survive/Born rules.\n");
    printf("\n");
    printf("                012345678\n");
    printf("  ------------------------\n");
    printf("  Survive     : ");
    for (i = 0; i < 9; i++) {
        printf("%d", lp->rule_survive[i]);
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
                            lp->rule_survive[curx] = !(lp->rule_survive[curx]);
                            printf("%d", lp->rule_survive[curx]); 
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
                        printf("%d", lp->rule_survive[i]);
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
                    if (n[cury] > BOARD_SIZE_MAX) n[cury] = BOARD_SIZE_MAX;
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
        "R-pentomino (F-pentomino)",
        "S23/B3",
        ".##",
        "##",
        ".#",
        "",
        // F-heptomino
        "F-heptomino",
        "S23/B3",
        "##",
        ".#",
        ".#",
        ".###",
        "",
        // Glider
        "Glider",
        "S23/B3",
        ".#",
        "..#",
        "###",
        "",
        // Spaceship
        "Spaceship (lightweight, middleweight, heavyweight)",
        "S23/B3",
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
        "S23/B3",
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
        "S23/B3",
        "......#",
        "##",
        ".#...###",
        "",
        // Acom
        "Acom",
        "S23/B3",
        ".#",
        "...#",
        "##..###",
        "",
        // Gosper Glider Gun
        "Gosper Glider Gun",
        "S23/B3",
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
        "S23/B3",
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
        "S23/B3",
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
        "S23/B3",
        "..#....#",
        "##.####.##",
        "..#....#",
        "",
        // The replicator
        "The replicator (S23/B36)",
        "S23/B36",
        ".###",
        "#",
        "#",
        "#",
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
        int str_n = 0, pattern_n = 1, p = 0, line;
        TextScreenBitmap *tempbitmap;
        int x, y;
        
        ClearPatternInfo(lp);
        TextScreen_ClearBitmap(lp->bitmap);
        
        // load pattern
        while (*preset_pattern[str_n]) {
            line = 1;
            while (*preset_pattern[str_n]) {
                if (cury == pattern_n) {
                    if (line == 1) {
                        snprintf_a(lp->name, sizeof(lp->name), "%s", preset_pattern[str_n]);
                    } else if (line == 2) {
                        SetRules(lp, preset_pattern[str_n]);
                    } else {
                        TextScreen_DrawText(lp->bitmap, 0, p++, preset_pattern[str_n]);
                    }
                }
                str_n++;
                line++;
            }
            pattern_n++;
            if (pattern_n > cury)
                break;
            str_n++;
        }
        snprintf_a(lp->comment, sizeof(lp->comment), "%s", "  Load from preset pattern.");
        
        // convert to internal format
        for (y = 0; y < lp->bitmap->height; y++) {
            for (x = 0; x < lp->bitmap->width; x++) {
                if (TextScreen_GetCell(lp->bitmap, x, y) == '#') {
                    TextScreen_PutCell(lp->bitmap, x, y, lp->surviveChar);
                } else {
                    TextScreen_ClearCell(lp->bitmap, x, y);
                }
            }
        }
        
        // move loaded pattern to center of bitmap
        CheckActive(lp);
        lp->patternWidth  = lp->active.right - lp->active.left + 1;
        lp->patternHeight = lp->active.bottom - lp->active.top + 1;
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
    char *helptext = "  [space]toggle [Enter]start [x]clear [v]paste [r]read [s]store [h]help";

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
                    ClearPatternInfo(lp);
                    break;
                case 'r':  // read
                    TextScreen_CopyBitmap(lp->bitmap, lp->storebitmap, 0, 0);
                    lp->genCount = 0;
                    EDITBOARD_SHOWCENTER();
                    ClearPatternInfo(lp);
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
                        if (TextScreen_GetCell(lp->bitmap, x, y) == lp->surviveChar) {
                            TextScreen_ClearCell(lp->bitmap, x, y);
                        } else {
                            TextScreen_PutCell(lp->bitmap, x, y, lp->surviveChar);
                        }
                        TextScreen_ShowBitmap(lp->bitmap, -lp->offsetx, -lp->offsety);
                    }
                    break;
                case 'i':  // show info
                    PrintInfo(lp);
                    TextScreen_SetCursorVisible(1);
                    TextScreen_ShowBitmap(lp->bitmap, -lp->offsetx, -lp->offsety);
                    EDITBOARD_SHOWHELPLINE();
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
            case 'i':
                PrintInfo(lp);
                TextScreen_ShowBitmap(lp->bitmap, -lp->offsetx, -lp->offsety);
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
    // set interrupt handler (Ctrl+C)
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
    
    // move cursor to end of line, and line feed
    TextScreen_SetCursorPos(0, lp.consoleHeight - 1);
    printf("\n");
    // free board
    FreeLifeParam(&lp);
    // free TextScreen
    TextScreen_End();
    
    return 0;
}

