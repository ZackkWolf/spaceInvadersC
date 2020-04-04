#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

void swap(int* first, int* second);
void clearScreen();
void plotPixel(int x, int y, short int lineColor);
void waitForVSync();
void drawLine(int x0, int y0, int x1, int y1, short int color);
void drawVerticalLine(int x, int y0, int y1, short int color);

//this function draws black from xInit to (xInit + width)
//and yInit to (yInit + height)
void drawBlack(int xInit, int yInit, int width, int height);

void drawPlayer(int xInit, int yInit);

void ps2Input(char b1, char b2, char b3, int* playerX, bool* shotFired);



const int PLAYER_WIDTH = 11;
const int PLAYER_HEIGHT = 8;

const int SCREEN_WIDTH = 320; //X
const int SCREEN_HEIGHT = 240; //Y

volatile int pixelBufferStart; // global variable

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
    const int playerY = SCREEN_HEIGHT - 20;

    int shotPositionX = SCREEN_WIDTH;
    int shotPositionY = SCREEN_HEIGHT / 2;
    short int shotColor = 0x0000;
    bool shotFired = false;
    bool eraseShot = false; // this variables keeps track of if you need to erase the shot on the next frame

    drawPlayer(playerX, playerY);
    waitForVSync(); // swap front and back buffers on VGA vertical sync
    pixelBufferStart = *(pixelCtrlPtr + 1); // new back buffer

    while (1) {
        // clear old things drawn (you are on the buffer that's one frame behind)

        // draw black on the left and right of the player to clear their trail
        drawBlack(playerX + PLAYER_WIDTH, playerY + 3, 2, 5);
        drawBlack(playerX - 2, playerY + 3, 2, 5);

        // a shot was fired on the last frame, so clear it
        if (shotColor != 0x0000) {
            drawVerticalLine(shotPositionX, shotPositionY + 4, shotPositionY + 7, 0x0000);

            //move the shot four pixels up if its not at the top of the screen
            //otherwise set its colour to black so it doesnt draw anymore
            if (shotPositionY > 10) {
                shotPositionY -= 4;
            }
            else {
                shotColor = 0x0000;
                eraseShot = true;
            }
        }
        else if (eraseShot) {
            drawVerticalLine(shotPositionX, shotPositionY, shotPositionY + 3, 0x0000);
            eraseShot = false;
        }

        //update variables to current frame and draw new frame
        volatile int PS2_data = *(PS2_ptr); // read the Data register in the PS/2 port

        byte1 = byte2;
        byte2 = byte3;
        byte3 = PS2_data & 0xFF;

        shotFired = false;

        //checks for input and updates variables accordingly
        ps2Input(byte1, byte2, byte3, &playerX, &shotFired);

        // initialize the shot if the button is pressed and if a shot is not already fired
        if (shotFired && shotColor == 0x0000) {
            shotPositionX = playerX + PLAYER_WIDTH / 2;
            shotPositionY = playerY - 5;
            shotColor = 0xFFFF;
            printf("%d\n", pixelBufferStart);
        }

        // if the shotColor is not black, then draw it
        if (shotColor != 0x0000) {
            drawVerticalLine(shotPositionX, shotPositionY, shotPositionY + 3, shotColor);
        }

        //draw the player in their current position
        drawPlayer(playerX, playerY);

        // swap buffers
        waitForVSync(); // swap front and back buffers on VGA vertical sync
        pixelBufferStart = *(pixelCtrlPtr + 1); // new back buffer

    }
}

void ps2Input(char b1, char b2, char b3, int* playerX, bool* shotFired) {
    volatile unsigned int twoParts;
    //volatile unsigned int breakCode;

    twoParts = (b2 << 8) | b3;
    //breakCode = (b1 << 16) | (b2 << 8) | b3;

    //Right Key
    //makecode = 0xe074, breakcode = 0xe0f074
    if (twoParts == 0xe074 && *playerX < (SCREEN_WIDTH - PLAYER_WIDTH)) {   	// Pressed Right and the player is not on the edge of the screen
        // move the player right
        (*playerX) += 2;
    }
    //Left Key
    //makecode = 0xe06b, breakcode = 0xe0f06b
    else if (twoParts == 0xe06b && *playerX > PLAYER_WIDTH) {                   // Pressed Left and the player is not on the edge of the screen
        // move the player left
        (*playerX) -= 2;
    }
    //Space Key
    //makecode = 0x29, breakcode = 0xf029
    else if (twoParts == 0xf029) {                                              // Released Space
        *shotFired = true;
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

void drawVerticalLine(int x, int y0, int y1, short int color) {
    if (y0 > y1) {
        swap(&y0, &y1);
    }

    for (int i = y0; i <= y1; ++i) {
        plotPixel(x, i, color);
    }
}

void drawLine(int x0, int y0, int x1, int y1, short int color) {
    bool isSteep = abs(y1 - y0) > abs(x1 - x0);

    if (isSteep) {
        swap(&x0, &y0);
        swap(&x1, &y1);
    }

    if (x0 > x1) {
        swap(&x0, &x1);
        swap(&y0, &y1);
    }

    int deltaX = x1 - x0;
    int deltaY = abs(y1 - y0);
    int error = -(deltaX / 2);
    int y = y0;
    int yStep;

    if (y0 < y1) { yStep = 1; }
    else { yStep = -1; }

    for (int x = x0; x <= x1; ++x) {
        if (isSteep) { plotPixel(y, x, color); }
        else { plotPixel(x, y, color); }

        error += deltaY;
        if (error >= 0) {
            y += yStep;
            error -= deltaX;
        }
    }
}
