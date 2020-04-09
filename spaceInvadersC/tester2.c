#include <stdio.h>

void disable_A9_interrupts(void);
void set_A9_IRQ_stack(void);
void config_GIC(void);
void config_KEYs(void);
void config_PS2(void);
void enable_A9_interrupts(void);
void pushbutton_ISR(void);
void ps2_ISR(void);
void config_interrupt(int, int);

/* ********************************************************************************
* This program demonstrates use of interrupts with C code. The program
*responds
* to interrupts from the pushbutton KEY port in the FPGA.
*
* The interrupt service routine for the KEYs indicates which KEY has been
*pressed
* on the LED display.
********************************************************************************/
int main(void) {
	disable_A9_interrupts(); // disable interrupts in the A9 processor
	set_A9_IRQ_stack(); // initialize the stack pointer for IRQ mode
	config_GIC(); // configure the general interrupt controller
	config_KEYs(); // configure pushbutton KEYs to generate interrupts
	config_PS2();
	enable_A9_interrupts(); // enable interrupts in the A9 processor
	while (1) // wait for an interrupt
		;
}
/* setup the KEY interrupts in the FPGA */
void config_KEYs() {
	volatile int* KEY_ptr = (int*)0xFF200050; // pushbutton KEY base address
	*(KEY_ptr + 2) = 0xF; // enable interrupts for the two KEYs
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
	if (interrupt_ID == 73) // check if interrupt is from the KEYs
		pushbutton_ISR();
	else if (interrupt_ID == 79)	// check if interrupt if from the PS2 Controller 1
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
	config_interrupt(73, 1); // configure the FPGA KEYs interrupt (73)
	config_interrupt(79, 1); // configure the FPGA PS2 interrupt (7)
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
/********************************************************************
* Pushbutton - Interrupt Service Routine
*
* This routine checks which KEY has been pressed. It writes to HEX0
*******************************************************************/
void pushbutton_ISR(void) {
	/* KEY base address */
	volatile int* KEY_ptr = (int*)0xFF200050;
	/* HEX display base address */
	volatile int* HEX3_HEX0_ptr = (int*)0xFF200020;
	int press, HEX_bits;
	press = *(KEY_ptr + 3); // read the pushbutton interrupt register
	*(KEY_ptr + 3) = press; // Clear the interrupt
	if (press & 0x1) // KEY0
		HEX_bits = 0b00111111;
	else if (press & 0x2) // KEY1
		HEX_bits = 0b00000110;
	else if (press & 0x4) // KEY2
		HEX_bits = 0b01011011;
	else // press & 0x8, which is KEY3
		HEX_bits = 0b01001111;
	*HEX3_HEX0_ptr = HEX_bits;
	return;
}
/********************************************************************
* Pushbutton - Interrupt Service Routine
*
* This routine checks which key has been pressed. It writes to HEX0
*******************************************************************/
void ps2_ISR(void) {
	volatile int* PS2_ptr = (int*)0xff200100;
	int PS2_data, RVALID;
	char byte1 = 0, byte2 = 0, byte3 = 0;

		PS2_data = *(PS2_ptr); // read the Data register in the PS/2 port
		RVALID = PS2_data & 0x8000; // extract the RVALID field
		if (RVALID) {
			/* shift the next data byte into the display */
			byte1 = byte2;
			byte2 = byte3;
			byte3 = PS2_data & 0xFF;

			volatile int* HEX3_HEX0_ptr = (int*)0xFF200020;
			volatile int* HEX5_HEX4_ptr = (int*)0xFF200040;
			/* SEVEN_SEGMENT_DECODE_TABLE gives the on/off settings for all segments in
			* a single 7-seg display in the DE1-SoC Computer, for the hex digits 0 - F
			*/
			unsigned char seven_seg_decode_table[] = {
				0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07,
				0x7F, 0x67, 0x77, 0x7C, 0x39, 0x5E, 0x79, 0x71 };
			unsigned char hex_segs[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
			unsigned int shift_buffer, nibble;
			unsigned char code;
			int i;
			shift_buffer = (byte1 << 16) | (byte2 << 8) | byte3;
			for (i = 0; i < 6; ++i) {
				nibble = shift_buffer & 0x0000000F; // character is in rightmost nibble
				code = seven_seg_decode_table[nibble];
				hex_segs[i] = code;
				shift_buffer = shift_buffer >> 4;
			}
			/* drive the hex displays */
			*(HEX3_HEX0_ptr) = *(int*)(hex_segs);
			*(HEX5_HEX4_ptr) = *(int*)(hex_segs + 4);
		}
	return;
}
