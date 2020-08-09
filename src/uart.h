/*_______________________________________________________________________________

 UART.h v2.2

 Uses 8 data bits, 1 stop bit, no parity

 - USED PERIPHERALS
 Timer 1

 - TESTED DEVICES:
 EFM8BB3

 For complete details about the library, visit
 www.programming-electronics-diy.xyz

 NOTICE
 --------
 NO PART OF THIS WORK CAN BE COPIED, DISTRIBUTED OR PUBLISHED WITHOUT A
 WRITTEN PERMISSION FROM THE AUTHOR. THE LIBRARY, NOR ANY PART
 OF IT CAN BE USED IN COMMERCIAL APPLICATIONS. IT IS INTENDED TO BE USED FOR
 HOBBY, LEARNING AND EDUCATIONAL PURPOSE ONLY. IF YOU WANT TO USE THEM IN
 COMMERCIAL APPLICATION PLEASE WRITE TO THE AUTHOR.

 Email: istrateliviu24@yahoo.com

 Copyright (c) 2018 I Liviu
 __________________________________________________________________________________*/

#ifndef UART_H_
#define UART_H_

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <SI_EFM8BB1_Register_Enums.h> // SFR declarations

//-----------------------------------------------------------------------------
// Defines
//-----------------------------------------------------------------------------
#define UART_TX_BUFFER_SIZE				40 // Max 256 Bytes
#define UART_RX_BUFFER_SIZE				80 // Max 256 Bytes

//-----------------------------------------------------------------------------
// Global VARIABLES
//-----------------------------------------------------------------------------
static volatile uint8_t xdata UART_TX_BUFFER[UART_TX_BUFFER_SIZE] = {'\0'};
volatile uint8_t xdata UART_RX_BUFFER[UART_RX_BUFFER_SIZE] = {'\0'};
static volatile uint8_t TX_BYTES_SENT = 0;
static volatile uint8_t TX_CURRENT_INDEX = 0;
static volatile uint8_t RX_BYTES_RECEIVED = 0;
static volatile uint8_t RX_BYTES_READ = 0;
static volatile bool RX_BUFFER_OVERFLOW = false;
static volatile bool TX_TRANSMITTING = false;
volatile bool UART_INTERRUPT = false;
static uint8_t UART_DATA_LENGTH = 0;
static bool UART_INITIATED = false;

//-----------------------------------------------------------------------------
// Function PROTOTYPES
//-----------------------------------------------------------------------------
void UART_Begin(uint32_t baudrate);
void UART_End(void);
void UART_Send(char *s);
void UART_SendSize(char *s, uint8_t size);
const char *UART_Read(void);
uint8_t UART_ReadByte(void);
bool UART_Available(bool timeout);

//-----------------------------------------------------------------------------
// Functions
//-----------------------------------------------------------------------------
void UART_Begin(uint32_t baudrate) {
	// *******************************
	// Baud rate calculation
	//********************************
	// Supported baud rates: 9600, 14400, 19200, 38400, 57600, 115200, 128000, 256000
	// For best results, choose baud rates that are accurate to +/-1% or better

	uint8_t xdata
	timer1_reload_value = 0;
	uint8_t xdata
	prescaler = 1;

	if (UART_INITIATED)
		return;

#if F_CPU == 49000000
	if(baudrate < 57600) {
		if(baudrate < 19200) {
			prescaler = 12;
		} else {
			prescaler = 4;
		}
	}
#elif F_CPU == 24500000
	if(baudrate < 57600) {
		if(baudrate < 19200) {
			prescaler = 12;
		} else {
			prescaler = 4;
		}
	}
#elif F_CPU == 16000000
	if(baudrate < 38400) {
		if(baudrate < 19200) {
			prescaler = 12;
		} else {
			prescaler = 4;
		}
	}
#elif F_CPU == 8000000
	// If baud rate is 256000 the error will be -2.34%
	if(baudrate < 19200) {
		prescaler = 4;
	}
#elif F_CPU == 1000000
	// If baud rate is 57600 the error will be -3.54%
	// If baud rate is 115200 the error will be 8.5%
	// If baud rate is 128000 or 256000 the error will be -2.34%
	prescaler = 1;
#endif

	timer1_reload_value = 256 - (((F_CPU / prescaler) / baudrate) / 2);

	// *******************************
	// Setup of pins and crossbar
	//********************************
	// This selects digital mode for the TX and RX pins
	P0MDIN |= (1 << 4) | (1 << 5);

	// This configures the pin as push-pull
	P0MDOUT |= (1 << 4); // only for TX pin

	// This configures the pin as open-drain
	P0MDOUT &= ~(1 << 5); // only for RX pin
	P0 |= (1 << 5); // RX pin HIGH

	// Don't skip this pins when assigning peripherals
	P0SKIP &= ~((1 << 4) | (1 << 5));

	// UART TX, RX routed to Port pins P0.4 and P0.5 (cannot be changed)
	XBR0 |= (1 << 0);

	// *******************************
	// Timer 1 setup, used by the UART
	//********************************
	// Stop Timer1 in case it was running
	TCON &= ~(1 << 6);

	// Timer 1 uses the clock defined by the prescale field, SCA
	CKCON0 &= ~(1 << 3);

	// Set prescale
	if (prescaler == 1) {
		CKCON0 |= (1 << 3); // Timer 1 uses the system clock
	} else if (prescaler == 4) {
		CKCON0 |= (1 << 0); // System clock divided by 4
	} else if (prescaler == 12) {
		CKCON0 &= ~(1 << 0); // System clock divided by 12
	}

	// Mode 2, 8-bit Counter/Timer with Auto-Reload
	TMOD |= (1 << 5);

	// Set reload value
	TL1 = timer1_reload_value;
	TH1 = timer1_reload_value;

	// Start Timer1
	TCON |= (1 << 6);

	// *******************************
	// UART setup
	//********************************
	// UART0 reception enabled
	SCON0 |= (1 << 4);
	// RI is set and an interrupt is generated only when the stop bit
	// is logic 1 (Mode 0) or when the 9th bit is logic 1 (Mode 1)
	SCON0 |= (1 << 5);

	// *******************************
	// Calculate Timer1 Overflow Rate in microseconds
	//********************************
	//TIMER1_OVERFLOW_TIME = 1000000 / (baudrate * 2);

	// *******************************
	// Interrupts
	//********************************
	IE_EA = 1; // Enable global interrupts
	IE_ES0 = 1; // Enable UART0 interrupt

	UART_INITIATED = true;
	;
}

void UART_End(void) {
	// Disable UART0 interrupt
	IE_ES0 = 0;

	// UART TX, RX is NOT routed to Port pins P0.4 and P0.5
	XBR0 &= ~(1 << 0);

	// Stop Timer1
	TCON &= ~(1 << 6);
}

void UART_Send(char* *s) {
	uint16_t timeout = 0;
	uint8_t i = 0;

	// Put new data in buffer only if the ISR finished sending the data in the buffer or if this is the first transmission.
	// Wait for the ISR to complete transmission. In case the interrupt or UART is somehow disabled,
	// exit the loop after about 10ms at 115200 baud rate
	while (TX_TRANSMITTING) {
		// Check if Timer 1 is enabled otherwise break the loop
		if (TCON & (1 << 6)) {
			// Wait for Timer1 overflow
			while (TCON_TF1 == 0)
				;
			// Reset timer1 overflow flag
			TCON_TF1 = 0;
			timeout++;
			if (timeout > 2304) {
				break;
			}
		} else {
			break;
		}
	}

	// Put the data in buffer
	while (*s || i < UART_DATA_LENGTH) {

		// If the buffer is full, start from the beginning. - 1 because index starts from 0
		if (TX_CURRENT_INDEX > UART_TX_BUFFER_SIZE - 1) {
			// Set the flag
			TX_TRANSMITTING = true;

			// Trigger TX interrupt
			SCON0_TI = 1;

			// Wait for the UART interrupt to transmit the data
			while (TX_TRANSMITTING) {
			}
		}

		UART_TX_BUFFER[TX_CURRENT_INDEX] = *s;

		TX_CURRENT_INDEX++;
		s++;
		i++;
	}

	UART_DATA_LENGTH = 0;

	// Set the flag
	TX_TRANSMITTING = true;

	// Trigger TX interrupt
	SCON0_TI = 1;
}

void UART_SendSize(char *s, uint8_t size) {
	UART_DATA_LENGTH = size;
	UART_Send(s);
}

// Returns the address of the UART receive buffer to access the received data
const char *UART_Read(void) {
	return UART_RX_BUFFER;
}

uint8_t UART_ReadByte(void) {
	uint8_t received_byte = '\0';

	received_byte = UART_RX_BUFFER[RX_BYTES_READ];

	RX_BYTES_READ++;

	// Returns NULL when no new data is available
	return received_byte;
}

// Returns 1 if new data is available
bool UART_Available(bool timeout) {
	uint16_t timer1_overflow_count = 0;

	UART_INTERRUPT = false;

	// Exit the function if after about 32ms at 115200 baud rate, UART doesn't receive any more data
	while (timer1_overflow_count < 7360) {
		// UART received at least 1 byte
		if ((RX_BYTES_READ < RX_BYTES_RECEIVED) || RX_BUFFER_OVERFLOW) {
			if (RX_BYTES_READ > UART_RX_BUFFER_SIZE - 2) {
				RX_BUFFER_OVERFLOW = false;
				RX_BYTES_READ = 0;
			}

			return true;
		}

		// Skip the timeout if not required
		if (timeout == false)
			return false;

		// Wait for Timer1 overflow
		while (TCON_TF1 == 0)
			;

		// Reset Timer1 overflow flag
		TCON_TF1 = 0;

		timer1_overflow_count++;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Interrupts
//-----------------------------------------------------------------------------
SI_INTERRUPT(UART0_ISR, UART0_IRQn) {
	// Transmission Interrupt
	if(SCON0_TI) {
		if(TX_BYTES_SENT < TX_CURRENT_INDEX) {
			SBUF0 = UART_TX_BUFFER[TX_BYTES_SENT];
			TX_BYTES_SENT++;
		} else {
			TX_BYTES_SENT = 0;
			TX_CURRENT_INDEX = 0;
			TX_TRANSMITTING = false;
		}

		// Clear TX flag
		SCON0_TI = 0;

		// Reception Interrupt
	} else if(SCON0_RI) {
		UART_INTERRUPT = true;

		// Check if the buffer is full
		if(RX_BYTES_RECEIVED > UART_RX_BUFFER_SIZE - 2) { // substract 1 for NULL char and 1 because of index
			RX_BYTES_RECEIVED = 0;
			RX_BUFFER_OVERFLOW = true;
		}

		UART_RX_BUFFER[RX_BYTES_RECEIVED] = SBUF0;
		RX_BYTES_RECEIVED++;

		// Clear RX flag
		SCON0_RI = 0;
	}
}

#endif /* UART_H_ */
