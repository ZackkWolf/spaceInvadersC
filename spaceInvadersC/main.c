#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

void swap(int* first, int* second);
void clearScreen();
void plotPixel(int x, int y, short int lineColor);
void waitForVSync();

//this function draws black from xInit to (xInit + width)
//and yInit to (yInit + height)
void drawBlack(int xInit, int yInit, int width, int height);

void drawPlayer(int xInit, int yInit);
const int PLAYER_WIDTH = 11;
const int PLAYER_HEIGHT = 8;

const int SCREEN_WIDTH = 320; //X
const int SCREEN_HEIGHT = 240; //Y

volatile int pixelBufferStart; // global variable
const int n = 8;

const short int playerIcon[8][11] = {
{0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x4FE0, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000},
    {0x0000, 0x0000, 0x0000, 0x0000, 0x4FE0, 0x4FE0, 0x4FE0, 0x0000, 0x0000, 0x0000, 0x0000},
    {0x0000, 0x0000, 0x0000, 0x0000, 0x4FE0, 0x4FE0, 0x4FE0, 0x0000, 0x0000, 0x0000, 0x0000},
    {0x0000, 0x4FE0, 0x4FE0, 0x4FE0, 0x4FE0, 0x4FE0, 0x4FE0, 0x4FE0, 0x4FE0, 0x4FE0, 0x0000},
    {0x4FE0, 0x4FE0, 0x4FE0, 0x4FE0, 0x4FE0, 0x4FE0, 0x4FE0, 0x4FE0, 0x4FE0, 0x4FE0, 0x4FE0},
    {0x4FE0, 0x4FE0, 0x4FE0, 0x4FE0, 0x4FE0, 0x4FE0, 0x4FE0, 0x4FE0, 0x4FE0, 0x4FE0, 0x4FE0},
    {0x4FE0, 0x4FE0, 0x4FE0, 0x4FE0, 0x4FE0, 0x4FE0, 0x4FE0, 0x4FE0, 0x4FE0, 0x4FE0, 0x4FE0},
    {0x4FE0, 0x4FE0, 0x4FE0, 0x4FE0, 0x4FE0, 0x4FE0, 0x4FE0, 0x4FE0, 0x4FE0, 0x4FE0, 0x4FE0}
};

int main(void) {
    volatile int* pixelCtrlPtr = (int*)0xFF203020;

    /* set front pixel buffer to start of FPGA On-chip memory */
    *(pixelCtrlPtr + 1) = 0xC8000000; // first store the address in the 
                                      // back buffer

    /* now, swap the front/back buffers, to set the front buffer location */
    waitForVSync();
    /* initialize a pointer to the pixel buffer, used by drawing functions */
    pixelBufferStart = *pixelCtrlPtr;
    clearScreen(); // pixelBufferStart points to the pixel buffer
    /* set back pixel buffer to start of SDRAM memory */
    *(pixelCtrlPtr + 1) = 0xC0000000;
    pixelBufferStart = *(pixelCtrlPtr + 1); // we draw on the back buffer

    clearScreen();

    // continually draw the screen
    while (1) {
        drawPlayer(80, 50);


        waitForVSync(); // swap front and back buffers on VGA vertical sync
        pixelBufferStart = *(pixelCtrlPtr + 1); // new back buffer
    }
}

void drawPlayer(int xInit, int yInit) {
    for (int y = 0; y < PLAYER_HEIGHT; ++y) {
        for (int x = 0; x < PLAYER_WIDTH; ++x) {
            plotPixel(xInit + x, yInit + y, playerIcon[y][x]);
        }
    }
}

void drawBlack(int xInit, int yInit, int width, int height) {
    for (int x = xInit; x < xInit + width && x < SCREEN_WIDTH; ++x) {
        for (int y = yInit; y < yInit + height && y < SCREEN_HEIGHT; ++y) {
            plotPixel(x, y, 0x000000);
        }
    }
}


void plotPixel(int x, int y, short int lineColor) {
    *(short int*)(pixelBufferStart + (y << 10) + (x << 1)) = lineColor;
}

void waitForVSync() {
    volatile int* pixelCtrlPtr = (int*)0xFF203020;	// pixel ctrl
    register int status;

    *pixelCtrlPtr = 1;	// start sync process
    status = *(pixelCtrlPtr + 3);	//status reg of DMA ctrl

    while ((status & 0x01) != 0) {
        status = *(pixelCtrlPtr + 3);
    }
}

void clearScreen() {
    short int black = 0;
    for (int i = 0; i < SCREEN_WIDTH; i++) {
        for (int j = 0; j < SCREEN_HEIGHT; j++) {
            plotPixel(i, j, black);
        }
    }
}

void swap(int* first, int* second) {
    int temp = *first;
    *first = *second;
    *second = temp;
}

