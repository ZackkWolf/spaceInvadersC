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

bool HEX_PS2(char b1, char b2, char b3, int* playerX, int* playerY);



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
    *(pixelCtrlPtr + 1) = 0xC8000000;
    waitForVSync();
    pixelBufferStart = *pixelCtrlPtr;
    clearScreen();
    *(pixelCtrlPtr + 1) = 0xC0000000;
    pixelBufferStart = *(pixelCtrlPtr + 1);
    clearScreen();

    volatile int* PS2_ptr = (int*)0xff200100;
    volatile char byte1 = 0, byte2 = 0, byte3 = 0;

    *(PS2_ptr) = 0xFF; // reset

    //draw inital screen
    int playerX = SCREEN_WIDTH / 2;
    int playerY = SCREEN_HEIGHT - 20;

    drawPlayer(playerX, playerY);
    waitForVSync(); // swap front and back buffers on VGA vertical sync
    pixelBufferStart = *(pixelCtrlPtr + 1); // new back buffer

    // continually draw the screen
    while (1) {
        volatile int PS2_data = *(PS2_ptr); // read the Data register in the PS/2 port

        byte1 = byte2;
        byte2 = byte3;
        byte3 = PS2_data & 0xFF;

        //HEX_PS2 returns true if something was pressed and the screen needs to be drawn again
        if (HEX_PS2(byte1, byte2, byte3, &playerX, &playerY)) {
            waitForVSync(); // swap front and back buffers on VGA vertical sync
            pixelBufferStart = *(pixelCtrlPtr + 1); // new back buffer
        }
    }
}

bool HEX_PS2(char b1, char b2, char b3, int* playerX, int* playerY) {
    volatile unsigned int shift_buffer;

    shift_buffer = (b1 << 16) | (b2 << 8) | b3;

    if (shift_buffer == 0xe0f074 && *playerX < (SCREEN_WIDTH - PLAYER_WIDTH)) {   	// Pressed Right and the player is not on the edge of the screen

        //drawing on the previous screen, so the player is still
        //one pixel back
        drawBlack(*playerX - 2, *playerY + 3, 4, 5);

        // move the player right and draw it
        (*playerX) += 2;
        drawPlayer(*playerX, *playerY);
        return true;
    }
    else if (shift_buffer == 0xe0f06b && *playerX > PLAYER_WIDTH) {                 // Pressed Left and the player is not on the edge of the screen
        //drawing on the previous screen, so the player is still
        //one pixel back
        drawBlack(*playerX + PLAYER_WIDTH - 2, *playerY + 3, 4, 5);

        // move the player right and draw it
        (*playerX) -= 2;
        drawPlayer(*playerX, *playerY);
        return true;
    }
    return false;
}

void drawPlayer(int xInit, int yInit) {
    for (int y = 0; y < PLAYER_HEIGHT; ++y) {
        for (int x = 0; x < PLAYER_WIDTH; ++x) {
            plotPixel(xInit + x, yInit + y, playerIcon[y][x]);
        }
    }
}

void drawBlack(int xInit, int yInit, int width, int height) {
    for (int y = yInit; y < yInit + height && y < SCREEN_HEIGHT; ++y) {
        for (int x = xInit; x < xInit + width && x < SCREEN_WIDTH; ++x) {
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

