/*****************************************
 waveview.c
 
 Read and view WAV file.
     (c)2016 Programming by Coffey
     Date: 20160420
 
 build command
 (Windows) gcc waveview.c textscreen.c -lm -o waveview.exe
 (Linux  ) gcc waveview.c textscreen.c -lm -o waveview.out
 *****************************************/

// MSVC: ignore C4996 warning (fopen -> fopen_s etc...)
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

#ifdef _MSC_VER
#define  snprintf _snprintf
#endif

#include "textscreen.h"

// like WAVEFORMATEX (windows)
typedef struct {
    int  wFormatTag;
    int  nChannels;
    int  nSamplesPerSec;
    int  nAvgBytesPerSec;
    int  nBlockAlign;
    int  wBitsPerSample;
    int  cbSize;
    int  peakLevel[64];
    int  length;
} WaveFormat;

int read4b(FILE *fp, uint32_t *d)
{
    int  c;
    *d = 0;
    
    // read 4 byte with little endian
    c = fgetc(fp);
    if (c == EOF) return -1;
    *d += (uint32_t)c;
    c = fgetc(fp);
    if (c == EOF) return -1;
    *d += (uint32_t)c << 8;
    c = fgetc(fp);
    if (c == EOF) return -1;
    *d += (uint32_t)c << 16;
    c = fgetc(fp);
    if (c == EOF) return -1;
    *d += (uint32_t)c << 24;
    return 0;
}

int read2b(FILE *fp, uint16_t *d)
{
    int  c;
    *d = 0;
    
    // read 2 byte with little endian
    c = fgetc(fp);
    if (c == EOF) return -1;
    *d += (uint16_t)c;
    c = fgetc(fp);
    if (c == EOF) return -1;
    *d += (uint16_t)c << 8;
    return 0;
}

int read1b(FILE *fp, uint8_t *d)
{
    int  c;
    *d = 0;
    
    // read 1 byte
    c = fgetc(fp);
    if (c == EOF) return -1;
    *d += (uint8_t)c;
    return 0;
}

uint32_t str2fourcc(char *str)
{
    uint32_t fourcc = 0;
    
    fourcc += (uint32_t)str[0];
    fourcc += (uint32_t)str[1] << 8;
    fourcc += (uint32_t)str[2] << 16;
    fourcc += (uint32_t)str[3] << 24;
    return fourcc;
}

int read_wavedata(char *filename, WaveFormat *wf, TextScreenBitmap *bitmap)
{
    FILE *fp;
    uint32_t d32, totalsize, size, pos, fourcc;
    uint16_t d16;
    uint8_t  d8;
    int  fmt = 0;
    
    fp = fopen(filename, "rb");
    if (fp == NULL ) {
        printf("%s could not open.(exist ?)\n", filename);
        return -1;
    }
    
    printf("Reading %s : \n", filename);
    
    // check fourCC for WAV format
    read4b(fp, &fourcc);
    if (!(fourcc == str2fourcc("RIFF"))) {
        printf("This is not WAV file.(fourCC RIFF not found)\n");
        fclose(fp);
        return -1;
    }
    read4b(fp, &totalsize);
    read4b(fp, &fourcc);
    if (!(fourcc == str2fourcc("WAVE"))) {
        printf("This is not WAV file.(fourCC WAVE not found)\n");
        fclose(fp);
        return -1;
    }
    
    while (!read4b(fp, &fourcc)) {
        if (read4b(fp, &size)) {
           printf("Error: Broken WAV file?\n");
           fclose(fp);
           return -1;
        }
        if (fourcc == str2fourcc("fmt ")) {  // format chunk
            pos = 0;
            if (pos < size) { read2b(fp, &d16); wf->wFormatTag = d16; pos += 2; }
            if (pos < size) { read2b(fp, &d16); wf->nChannels = d16; pos += 2; }
            if (pos < size) { read4b(fp, &d32); wf->nSamplesPerSec = d32; pos += 4; }
            if (pos < size) { read4b(fp, &d32); wf->nAvgBytesPerSec = d32; pos += 4; }
            if (pos < size) { read2b(fp, &d16); wf->nBlockAlign = d16; pos += 2; }
            if (pos < size) { read2b(fp, &d16); wf->wBitsPerSample = d16; pos += 2; }
            if (pos < size) { read2b(fp, &d16); wf->cbSize = d16; pos += 2; }
            for (; pos < size; pos++) {
                if (read1b(fp, &d8)) {
                    fclose(fp);
                    return -1;
                }
            }
            fmt = 1;
        } else if (fourcc == str2fourcc("data")) {  // data chunk
            int16_t sdata[64];
            int smax[64] = {0};
            int i, x, s, tmp;
            int height, disprate = 2;
            
            // check error
            if (fmt == 0) {
                printf("Error: fmt chunk not found.\n");
                fclose(fp);
                return -1;
            }
            if (wf->wBitsPerSample != 16) {
                printf("This program works only 16bit depth.\n");
                fclose(fp);
                return -1;
            }
            if (wf->nChannels >= 64) {
                printf("WAV file has too many channels.\n");
                fclose(fp);
                return -1;
            }
            
            height = (bitmap->height) / 2 - 1;
            pos = 0;
            s = 0;
            x = 0;
            while(pos < size) {
                // read wav data
                for (i = 0; i < wf->nChannels; i++) {
                    if (read2b(fp, (uint16_t *)&sdata[i])) {  // read by S16le
                        printf("Error: Broken WAV file.\n");
                        fclose(fp);
                        return -1;
                    }
                    tmp = sdata[i];
                    if (tmp < 0) tmp = -tmp;
                    if (smax[i] < tmp) smax[i] = tmp;
                    pos += 2;
                }
                s++;
                if (s >= wf->nSamplesPerSec / disprate) {  // drawing bar in 'disprate' times per second
                    for (i = 0; i < 2 && i < wf->nChannels; i++) {  // draw 1 or 2 channels
                        char buf[32];
                        
                        // draw peak bar
                        if (smax[i] >= 32768) smax[i] = 32767;
                        TextScreen_DrawLine(bitmap, x, height*(i+1) ,
                                                    x, height*(i+1) - (smax[i] * (height-1) / 32768), '#');
                        
                        if (!(x % (10 * disprate))) { // draw time label
                            snprintf(buf, sizeof(buf) - 1, "^%d:%02d", (x/disprate) / 60, (x/disprate) % 60);
                            buf[sizeof(buf) - 1] = 0;
                            TextScreen_DrawText(bitmap, x, height*(i+1) + 1, buf);
                            if (i == 0) {
                                printf(".");
                                fflush(stdout);
                            }
                        }
                        if (wf->peakLevel[i] < smax[i]) wf->peakLevel[i] = smax[i];
                        smax[i] = 0;
                    }
                    s = 0;
                    x++;
                }
            }
            wf->length = size / wf->nChannels / 2;
        } else {  // not 'fmt ', 'data' chunk
            // read through
            for (pos = 0; pos < size; pos++) {
                if (read1b(fp, &d8)) {
                    printf("Error: Broken WAV file.\n");
                    fclose(fp);
                    return -1;
                }
            }
        }
    }
    
    fclose(fp);
    printf("done\n");
    return 0;
}

int main(int argc, char *argv[])
{
    WaveFormat wf;
    TextScreenBitmap *bitmap, *wavmap;
    char *helptext = "[Arrow key]scroll  [Home]reset view  [q][Esc]quit";
    char buf[256];
    int  x;
    int  key, redraw;
    int  ret;
    
    if (argc != 2) {
        printf("Usage: %s <WAV file name>\n", argv[0]);
        return -1;
    }
    
    // initialize TextScreen
    TextScreen_Init(0);
    bitmap = TextScreen_CreateBitmap(0, 0);
    wavmap = TextScreen_CreateBitmap(20000, bitmap->height - 2);
    memset(&wf, 0, sizeof(wf));
    
    // read wave data
    ret = read_wavedata(argv[1], &wf, wavmap);
    if (ret) {
        TextScreen_FreeBitmap(wavmap);
        TextScreen_FreeBitmap(bitmap);
        TextScreen_End();
        return -1;
    }
    
    // clear screen, hide cursor
    TextScreen_ClearScreen();
    TextScreen_SetCursorVisible(0);
    x = 0;
    key = 0;
    redraw = 1;
    
    // main loop
    while ((key != 'q') && (key != TSK_ESC)) {  // q or Esc key then quit
        if (key) {
            switch (key) {
                case TSK_ARROW_LEFT:   // left key
                    x += 10;
                    if (x > 0) x = 0;
                    break;
                case TSK_ARROW_RIGHT:  // right key
                    x -= 10;
                    break;
                case TSK_HOME:         // home key
                    x = 0;
                    break;
            }
            redraw = 1;
        }
        // redraw screen
        if (redraw) {
            TextScreen_ClearBitmap(bitmap);
            TextScreen_OverlayBitmap(bitmap, wavmap, x, 1);
            snprintf(buf, sizeof(buf) - 1, "WAVE VIEW(peak) : %s", argv[1]);
            buf[sizeof(buf) - 1] = 0;
            TextScreen_DrawText(bitmap, 0, 0, buf);
            snprintf(buf, sizeof(buf) - 1, "Samples %d (%02d:%02d.%03d) : Peak %.1fdB / %.1fdB : Rate %dHz",
                wf.length, wf.length / wf.nSamplesPerSec / 60, (wf.length / wf.nSamplesPerSec) % 60,
                (wf.length % wf.nSamplesPerSec) * 1000 / wf.nSamplesPerSec,
                20*log10((wf.peakLevel[0] ? (double)wf.peakLevel[0] : 0.5)/32768),
                20*log10((wf.peakLevel[1] ? (double)wf.peakLevel[1] : 0.5)/32768),
                wf.nSamplesPerSec);
            buf[sizeof(buf) - 1] = 0;
            TextScreen_DrawText(bitmap, 0, 1, buf);
            if (wf.nChannels == 2) {
                TextScreen_DrawText(bitmap, 0, (wavmap->height/2-1)+1, "LEFT");
                TextScreen_DrawText(bitmap, 0, (wavmap->height/2-1)*2+1, "RIGHT");
            }
            TextScreen_DrawText(bitmap, 0, bitmap->height - 1, helptext);
            TextScreen_ShowBitmap(bitmap, 0, 0);
            redraw = 0;
        }
        TextScreen_Wait(10);
        // check key input
        key = TextScreen_GetKey() & TSK_KEYMASK;
    }
    
    // move cursor to bottom line
    printf("\n");
    fflush(stdout);
    // free memory
    TextScreen_FreeBitmap(wavmap);
    TextScreen_FreeBitmap(bitmap);
    // finish TextScreen
    TextScreen_SetCursorVisible(1);
    TextScreen_End();
    
    return 0;
}

