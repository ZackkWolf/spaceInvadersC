#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

void swap(int* first, int* second);
void clearScreen();
void plotPixel(int x, int y, short int lineColor);
void drawLine(int x0, int y0, int x1, int y1, short int color);
void waitForVSync();
void drawBoxesAndLines(short colorBox[], int dxBox[], int dyBox[], int xBox[], int yBox[]);
void drawBox(int x, int y, short color);
void updateBoxLocations(int dxBox[], int dyBox[], int xBox[], int yBox[]);

int SCREEN_WIDTH = 320; //X
int SCREEN_HEIGHT = 240; //Y

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
    short colors[10] = { 0xFFFF, 0xF800, 0x07E0, 0x001F, 0xF1FF, 0xF0E0, 0x9E60, 0x07F7, 0x765F, 0x1058 };
    short colorBox[n];
    int dxBox[n];
    int dyBox[n];
    int xBox[n];
    int yBox[n];

    // initialize location and direction of rectangles
    for (int i = 0; i < n; ++i) {
        colorBox[i] = colors[rand() % 10];
        dxBox[i] = ((rand() % 2) * 2) - 1;
        dyBox[i] = ((rand() % 2) * 2) - 1;
        xBox[i] = rand() % SCREEN_WIDTH;
        yBox[i] = rand() % SCREEN_HEIGHT;
    }

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


    // continually draw the screen
    clearScreen();
    while (1) {

        int count = 0;
        for (int y = 0; y < 8; ++y) {
            for (int x = 0; x < 11; ++x) {
                plotPixel(x, y, playerIcon[y][x]);
                count++;
            }
        }

        waitForVSync(); // swap front and back buffers on VGA vertical sync
        pixelBufferStart = *(pixelCtrlPtr + 1); // new back buffer
    }
}

void drawBoxesAndLines(short colorBox[], int dxBox[], int dyBox[], int xBox[], int yBox[]) {
    for (int i = 0; i < n; ++i) {
        drawBox(xBox[i], yBox[i], colorBox[i]);
        drawLine(xBox[i], yBox[i], xBox[(i + 1) % n], yBox[(i + 1) % n], colorBox[i]);
    }
}

void drawBox(int x, int y, short color) {
    if ((x + 2 < SCREEN_WIDTH) && (x > 0) && (y + 2 < SCREEN_HEIGHT) && (y > 0)) {
        plotPixel(x, y, color);
        plotPixel(x, y + 1, color);
        plotPixel(x, y + 2, color);

        plotPixel(x + 1, y, color);
        plotPixel(x + 1, y + 1, color);
        plotPixel(x + 1, y + 2, color);

        plotPixel(x + 2, y, color);
        plotPixel(x + 2, y + 1, color);
        plotPixel(x + 2, y + 2, color);
    }
}

void updateBoxLocations(int dxBox[], int dyBox[], int xBox[], int yBox[]) {
    for (int i = 0; i < n; ++i) {
        xBox[i] += dxBox[i];
        yBox[i] += dyBox[i];

        if (xBox[i] + 3 >= SCREEN_WIDTH) {
            dxBox[i] = -1;
        }
        else if (xBox[i] <= 0) {
            dxBox[i] = 1;
        }

        if (yBox[i] + 3 >= SCREEN_HEIGHT) {
            dyBox[i] = -1;
        }
        else if (yBox[i] <= 0) {
            dyBox[i] = 1;
        }
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

