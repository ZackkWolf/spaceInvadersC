#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

void disable_A9_interrupts(void);
void set_A9_IRQ_stack(void);
void config_GIC(void);
void config_PS2(void);
void enable_A9_interrupts(void);
void ps2_ISR(void);
void config_interrupt(int, int);

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
void drawSquid0(int xInit, int yInit);
void drawSquid1(int xInit, int yInit);
void drawBunny0(int xInit, int yInit);
void drawBunny1(int xInit, int yInit);
void drawSkull0(int xInit, int yInit);
void drawSkull1(int xInit, int yInit);
void drawRedSplat(int xInit, int yInit);

void displayScoreOnHex3_0(int score);



const int PLAYER_WIDTH = 11;
const int PLAYER_HEIGHT = 8;

const int SCREEN_WIDTH = 320; //X
const int SCREEN_HEIGHT = 240; //Y

volatile int pixelBufferStart; // global variable

const char bitCodes[10] = { 0b00111111, 0b00000110, 0b01011011, 0b01001111, 0b01100110, 0b01101101, 0b01111101, 0b00000111, 0b01111111, 0b01100111 };

char byte1 = 0, byte2 = 0, byte3 = 0;

bool shotFired = false;
short int shotColor = 0x0000;

bool moveLeft = false;
bool moveRight = false;

int playerX = 320 / 2;
const int playerY = 240 - 20;


int main(void) {
	disable_A9_interrupts(); // disable interrupts in the A9 processor
	set_A9_IRQ_stack(); // initialize the stack pointer for IRQ mode
	config_GIC(); // configure the general interrupt controller
	config_PS2(); // configure the PS2 to enable interrupts
	enable_A9_interrupts(); // enable interrupts in the A9 processor



	volatile int* pixelCtrlPtr = (int*)0xFF203020;
	*(pixelCtrlPtr + 1) = 0xC8000000;
	waitForVSync();
	pixelBufferStart = *pixelCtrlPtr;
	clearScreen();
	drawPlayer(playerX, playerY);

	*(pixelCtrlPtr + 1) = 0xC0000000;
	pixelBufferStart = *(pixelCtrlPtr + 1);
	clearScreen();

	int shotPositionX = SCREEN_WIDTH;
	int shotPositionY = SCREEN_HEIGHT / 2;
	bool eraseShot = false; // this variables keeps track of if you need to erase the shot on the next frame

	int numShotsFired = 0;

	int redSplatPositionX;
	int redSplatPositionY;
	int redSplatFrames = 0;
	bool eraseRedSplat = false;

	bool drawLeft = false;
	bool drawRight = false;


	drawPlayer(playerX, playerY);
	waitForVSync(); // swap front and back buffers on VGA vertical sync
	pixelBufferStart = *(pixelCtrlPtr + 1); // new back buffer


	// put things that happen every frame regardless of user input here
	while (1) {
		if (moveLeft) {
			moveLeft = false;
			// draw black on the right of the player to clear their trail
			drawBlack(playerX + PLAYER_WIDTH - 2, playerY + 3, 4, 5);

			// move the player left
			(playerX) -= 2;
			drawPlayer(playerX, playerY);

			drawLeft = true;		// signal to update next screen
		}
		else if (drawLeft) {
			drawLeft = false;

			// draw black on the right of the player to clear their trail
			drawBlack(playerX + PLAYER_WIDTH, playerY + 3, 2, 5);

			drawPlayer(playerX, playerY);
		}
		else if (moveRight) {
			moveRight = false;
			// draw black on the left of the player to clear their trail
			drawBlack(playerX - 2, playerY + 3, 4, 5);

			// move the player right
			(playerX) += 2;
			drawPlayer(playerX, playerY);

			drawRight = true;		// signal to update next screen
		}
		else if (drawRight) {
			drawRight = false;

			// draw black on the left of the player to clear their trail
			drawBlack(playerX - 2, playerY + 3, 2, 5);

			drawPlayer(playerX, playerY);
		}

		// check if a shot has just been fired
		if (shotFired) {
			shotFired = false;	// turn the signal low

			// initialize the shot
			shotColor = 0xFFFF;
			shotPositionX = playerX + PLAYER_WIDTH / 2;
			shotPositionY = playerY - 5;


			// this is extra stuff for testing
			numShotsFired++;
			displayScoreOnHex3_0(numShotsFired);
			printf("%d\n", pixelBufferStart);
		}

		// check if a shot is in the air
		if (shotColor != 0x0000) {
			// clear the old shot
			drawVerticalLine(shotPositionX, shotPositionY + 4, shotPositionY + 7, 0x0000);

			// move the shot upwards if its not at the top of the screen
			// or erase the shot by drawing a red splat ontop of it at the top of the screen
			if (shotPositionY > 10) {
				// update its position and draw the shot
				shotPositionY -= 4;
				drawVerticalLine(shotPositionX, shotPositionY, shotPositionY + 3, shotColor);
			}
			else {
				//erase the shot
				shotColor = 0x0000;
				eraseShot = true;   //set this to true so the next frame erases the shot

				//draw the red splat
				redSplatPositionX = shotPositionX - PLAYER_WIDTH / 2;
				redSplatPositionY = shotPositionY;
				drawRedSplat(redSplatPositionX, redSplatPositionY);
				redSplatFrames = 1;
			}
		}
		else if (eraseShot) {   //enters this on the next frame to draw the red splat over the shot
			drawRedSplat(redSplatPositionX, redSplatPositionY);
			eraseShot = false;
		}

		// check if the red splat is being displayed
		if (redSplatFrames != 0) {
			//if its been displayed for 20 frames, erase it
			if (redSplatFrames > 20) {
				redSplatFrames = 0;
				drawBlack(redSplatPositionX, redSplatPositionY, 8, 8);
				eraseRedSplat = true;   //set this to true for the next frame to erase the red splat
			}
			else {
				redSplatFrames++;
			}
		}
		else if (eraseRedSplat) {
			drawBlack(redSplatPositionX, redSplatPositionY, 8, 8);
			eraseRedSplat = false;
		}

		// swap buffers
		waitForVSync(); // swap front and back buffers on VGA vertical sync
		pixelBufferStart = *(pixelCtrlPtr + 1); // new back buffer

	}
}

/********************************************************************
* Pushbutton - Interrupt Service Routine
*
* This routine checks which key has been pressed. It writes to HEX0
*******************************************************************/
void ps2_ISR(void) {
	volatile int* PS2_ptr = (int*)0xff200100;
	int PS2_data, RVALID;

	PS2_data = *(PS2_ptr); // read the Data register in the PS/2 port
	RVALID = PS2_data & 0x8000; // extract the RVALID field
	if (RVALID) {
		/* shift the next data byte into the display */
		byte1 = byte2;
		byte2 = byte3;
		byte3 = PS2_data & 0xFF;

		volatile unsigned int twoParts;
		//volatile unsigned int breakCode;

		twoParts = (byte2 << 8) | byte3;
		//breakCode = (b1 << 16) | (b2 << 8) | b3;

		//Right Key
		//makecode = 0xe074, breakcode = 0xe0f074
		if (twoParts == 0xe074 && playerX < (SCREEN_WIDTH - PLAYER_WIDTH)) {   	// Pressed Right and the player is not on the edge of the screen
			moveRight = true;
		}
		//Left Key
		//makecode = 0xe06b, breakcode = 0xe0f06b
		else if (twoParts == 0xe06b && playerX > PLAYER_WIDTH) {                // Pressed Left and the player is not on the edge of the screen
			moveLeft = true;
		}
		//Space Key
		//makecode = 0x29, breakcode = 0xf029
		else if (twoParts == 0xf029 && shotColor == 0x0000) {					// Released Space and there is not a shot already on screen
			shotFired = true;
		}
	}
	return;
}

void displayScoreOnHex3_0(int score) {
	int* hex3_0ptr = (int*)0xFF200020;
	int digits[4] = { 0,0,0,0 };    // digits is an array of the score digits from with entry 0 being the ones
	int bitCode = 0;

	for (int i = 0; i < 4; ++i) {
		digits[i] = score % 10;
		score /= 10;
	}

	// get the bit codes from most sig digit to least
	// and shift them by 4
	for (int i = 3; i >= 0; --i) {
		char code = bitCodes[digits[i]];
		bitCode = bitCode << 8 | code;
	}

	*hex3_0ptr = bitCode;
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

void drawBlack(int xInit, int yInit, int width, int height) {
	for (int y = yInit; y < yInit + height && y < SCREEN_HEIGHT; ++y) {
		for (int x = xInit; x < xInit + width && x < SCREEN_WIDTH; ++x) {
			plotPixel(x, y, 0x000000);
		}
	}
}

/* setup the PS2 interrupts in the FPGA */
void config_PS2(void) {
	volatile int* PS2_ptr = (int*)0xFF200100; // PS2 Controller 1 base address
	*(PS2_ptr + 1) = 0b1; // enable interrupts for the PS2 Controller
}
/* This file:
* 1. defines exception vectors for the A9 processor
* 2. provides code that sets the IRQ mode stack, and that dis/enables
* interrupts
* 3. provides code that initializes the generic interrupt controller
*/

// Define the IRQ exception handler
void __attribute__((interrupt)) __cs3_isr_irq(void) {
	// Read the ICCIAR from the CPU Interface in the GIC
	int interrupt_ID = *((int*)0xFFFEC10C);
	if (interrupt_ID == 79)	// check if interrupt is from the PS2 Controller 1
		ps2_ISR();
	else
		printf("Unexpeced interrupt with ID: %d", interrupt_ID);
	// Write to the End of Interrupt Register (ICCEOIR)
	*((int*)0xFFFEC110) = interrupt_ID;
}
// Define the remaining exception handlers
void __attribute__((interrupt)) __cs3_reset(void) {
	while (1);
}
void __attribute__((interrupt)) __cs3_isr_undef(void) {
	while (1);
}
void __attribute__((interrupt)) __cs3_isr_swi(void) {
	while (1);
}
void __attribute__((interrupt)) __cs3_isr_pabort(void) {
	while (1);
}
void __attribute__((interrupt)) __cs3_isr_dabort(void) {
	while (1);
}
void __attribute__((interrupt)) __cs3_isr_fiq(void) {
	while (1);
}
/*
* Turn off interrupts in the ARM processor
*/
void disable_A9_interrupts(void) {
	int status = 0b11010011;
	asm("msr cpsr, %[ps]" : : [ps] "r"(status));
}
/*
* Initialize the banked stack pointer register for IRQ mode
*/
void set_A9_IRQ_stack(void) {
	int stack, mode;
	stack = 0xFFFFFFFF - 7; // top of A9 onchip memory, aligned to 8 bytes
	/* change processor to IRQ mode with interrupts disabled */
	mode = 0b11010010;
	asm("msr cpsr, %[ps]" : : [ps] "r"(mode));
	/* set banked stack pointer */
	asm("mov sp, %[ps]" : : [ps] "r"(stack));
	/* go back to SVC mode before executing subroutine return! */
	mode = 0b11010011;
	asm("msr cpsr, %[ps]" : : [ps] "r"(mode));
}
/*
* Turn on interrupts in the ARM processor
*/
void enable_A9_interrupts(void) {
	int status = 0b01010011;
	asm("msr cpsr, %[ps]" : : [ps] "r"(status));
}
/*
* Configure the Generic Interrupt Controller (GIC)
*/
void config_GIC(void) {
	config_interrupt(79, 1); // configure the FPGA PS2 interrupt (79)
	// Set Interrupt Priority Mask Register (ICCPMR). Enable interrupts of all
	// priorities
	*((int*)0xFFFEC104) = 0xFFFF;
	// Set CPU Interface Control Register (ICCICR). Enable signaling of
	// interrupts
	*((int*)0xFFFEC100) = 1;
	// Configure the Distributor Control Register (ICDDCR) to send pending
	// interrupts to CPUs
	*((int*)0xFFFED000) = 1;
}
/*
* Configure Set Enable Registers (ICDISERn) and Interrupt Processor Target
* Registers (ICDIPTRn). The default (reset) values are used for other registers
* in the GIC.
*/
void config_interrupt(int N, int CPU_target) {
	int reg_offset, index, value, address;
	/* Configure the Interrupt Set-Enable Registers (ICDISERn).
	* reg_offset = (integer_div(N / 32) * 4
	* value = 1 << (N mod 32) */
	reg_offset = (N >> 3) & 0xFFFFFFFC;
	index = N & 0x1F;
	value = 0x1 << index;
	address = 0xFFFED100 + reg_offset;
	/* Now that we know the register address and value, set the appropriate bit */
	*(int*)address |= value;
	/* Configure the Interrupt Processor Targets Register (ICDIPTRn)
	* reg_offset = integer_div(N / 4) * 4
	* index = N mod 4 */
	reg_offset = (N & 0xFFFFFFFC);
	index = N & 0x3;
	address = 0xFFFED800 + reg_offset + index;
	/* Now that we know the register address and value, write to (only) the
	* appropriate byte */
	*(char*)address = (char)CPU_target;
}

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

const short int squidIcon0[8][8] = {
{0x0000, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0x0000, 0x0000, 0x0000},
	{0x0000, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0x0000},
	{0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000},
	{0xFFFF, 0xFFFF, 0x0000, 0xFFFF, 0xFFFF, 0x0000, 0xFFFF, 0xFFFF},
	{0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF},
	{0x0000, 0xFFFF, 0x0000, 0xFFFF, 0xFFFF, 0x0000, 0xFFFF, 0x0000},
	{0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF},
	{0x0000, 0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF, 0x0000}
};

const short int squidIcon1[8][8] = {
{0x0000, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0x0000, 0x0000, 0x0000},
	{0x0000, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0x0000},
	{0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000},
	{0xFFFF, 0xFFFF, 0x0000, 0xFFFF, 0xFFFF, 0x0000, 0xFFFF, 0xFFFF},
	{0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF},
	{0x0000, 0x0000, 0xFFFF, 0x0000, 0x0000, 0xFFFF, 0x0000, 0x0000},
	{0x0000, 0xFFFF, 0x0000, 0xFFFF, 0xFFFF, 0x0000, 0xFFFF, 0x0000},
	{0xFFFF, 0x0000, 0xFFFF, 0x0000, 0x0000, 0xFFFF, 0x0000, 0xFFFF}
};

const short int bunnyIcon0[8][11] = {
{0x0000, 0x0000, 0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF, 0x0000, 0x0000},
	{0x0000, 0x0000, 0x0000, 0xFFFF, 0x0000, 0x0000, 0x0000, 0xFFFF, 0x0000, 0x0000, 0x0000},
	{0x0000, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0x0000},
	{0x0000, 0xFFFF, 0xFFFF, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0xFFFF, 0xFFFF, 0x0000},
	{0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF},
	{0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF},
	{0xFFFF, 0x0000, 0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF, 0x0000, 0xFFFF},
	{0x0000, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0x0000, 0xFFFF, 0xFFFF, 0x0000, 0x0000, 0x0000}
};

const short int bunnyIcon1[8][11] = {
{0x0000, 0x0000, 0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF, 0x0000, 0x0000},
	{0xFFFF, 0x0000, 0x0000, 0xFFFF, 0x0000, 0x0000, 0x0000, 0xFFFF, 0x0000, 0x0000, 0xFFFF},
	{0xFFFF, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0xFFFF},
	{0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF},
	{0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF},
	{0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000},
	{0x0000, 0x0000, 0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF, 0x0000, 0x0000},
	{0x0000, 0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF, 0x0000}
};

const short int skullIcon0[8][12] = {
{0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000},
	{0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000},
	{0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF},
	{0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF},
	{0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF},
	{0x0000, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0x0000},
	{0x0000, 0xFFFF, 0xFFFF, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0x0000},
	{0x0000, 0x0000, 0xFFFF, 0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0x0000, 0x0000}
};

const short int skullIcon1[8][12] = {
{0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000},
	{0x0000, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000},
	{0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF},
	{0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0xFFFF},
	{0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF},
	{0x0000, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0x0000, 0x0000, 0xFFFF, 0xFFFF, 0x0000, 0x0000, 0x0000},
	{0x0000, 0x0000, 0xFFFF, 0xFFFF, 0x0000, 0xFFFF, 0xFFFF, 0x0000, 0xFFFF, 0xFFFF, 0x0000, 0x0000},
	{0xFFFF, 0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF, 0xFFFF}
};

const short int redSplat[8][8] = {
{0xF800, 0x0000, 0x0000, 0x0000, 0xF800, 0x0000, 0x0000, 0xF800},
	{0x0000, 0x0000, 0xF800, 0x0000, 0x0000, 0x0000, 0xF800, 0x0000},
	{0x0000, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0x0000},
	{0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800},
	{0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800},
	{0x0000, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0x0000},
	{0x0000, 0x0000, 0xF800, 0x0000, 0x0000, 0xF800, 0x0000, 0x0000},
	{0xF800, 0x0000, 0x0000, 0xF800, 0x0000, 0x0000, 0x0000, 0xF800}
};

void drawPlayer(int xInit, int yInit) {
	for (int y = 0; y < PLAYER_HEIGHT; ++y) {
		for (int x = 0; x < PLAYER_WIDTH; ++x) {
			plotPixel(xInit + x, yInit + y, playerIcon[y][x]);
		}
	}
}

void drawSquid0(int xInit, int yInit) {
	for (int y = 0; y < 8; ++y) {
		for (int x = 0; x < 8; ++x) {
			plotPixel(xInit + x, yInit + y, squidIcon0[y][x]);
		}
	}
}

void drawSquid1(int xInit, int yInit) {
	for (int y = 0; y < 8; ++y) {
		for (int x = 0; x < 8; ++x) {
			plotPixel(xInit + x, yInit + y, squidIcon1[y][x]);
		}
	}
}

void drawBunny0(int xInit, int yInit) {
	for (int y = 0; y < 8; ++y) {
		for (int x = 0; x < 11; ++x) {
			plotPixel(xInit + x, yInit + y, bunnyIcon0[y][x]);
		}
	}
}

void drawBunny1(int xInit, int yInit) {
	for (int y = 0; y < 8; ++y) {
		for (int x = 0; x < 11; ++x) {
			plotPixel(xInit + x, yInit + y, bunnyIcon1[y][x]);
		}
	}
}

void drawSkull0(int xInit, int yInit) {
	for (int y = 0; y < 8; ++y) {
		for (int x = 0; x < 12; ++x) {
			plotPixel(xInit + x, yInit + y, skullIcon0[y][x]);
		}
	}
}

void drawSkull1(int xInit, int yInit) {
	for (int y = 0; y < 8; ++y) {
		for (int x = 0; x < 12; ++x) {
			plotPixel(xInit + x, yInit + y, skullIcon1[y][x]);
		}
	}
}

void drawRedSplat(int xInit, int yInit) {
	for (int y = 0; y < 8; ++y) {
		for (int x = 0; x < 8; ++x) {
			plotPixel(xInit + x, yInit + y, redSplat[y][x]);
		}
	}
}
