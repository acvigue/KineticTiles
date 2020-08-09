/*
 * RGB Tile v1.4A code.
 * Copyright 2020
 * Written on: Aug 5, 2020
 * By: Aiden Vigue (acvig)
 */

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <SI_EFM8BB1_Register_Enums.h>                  // SFR declarations
#include <stdio.h>
#include <stdlib.h>
#define F_CPU   24500000
#include "InitDevice.h"
#include "ws2812.h"
#include "uart.h"

// $[Generated Includes]
// [Generated Includes]$

#define BAUD_RATE                       19200
#define RECEIVED_DATA_ARRAY_SIZE        10 // size of array where to store the incoming data

SI_SBIT(DET_C, SFR_P0, 6);
SI_SBIT(DET_A, SFR_P0, 1);
SI_SBIT(DET_B, SFR_P1, 5);

//-----------------------------------------------------------------------------
// SiLabs_Startup() Routine
// ----------------------------------------------------------------------------
// This function is called immediately after reset, before the initialization
// code is run in SILABS_STARTUP.A51 (which runs before main() ). This is a
// useful place to disable the watchdog timer, which is enable by default
// and may trigger before main() in some instances.
//-----------------------------------------------------------------------------
void SiLabs_Startup(void) {
    // $[SiLabs Startup]
    // [SiLabs Startup]$
}

//-----------------------------------------------------------------------------
// main() Routine
// ----------------------------------------------------------------------------
void main(void) {
    //declare all vars at top because compiled asm can't declare later in time; also ws2812 code uses bare assembly and declaring later will cause heap crash :(
    uint8_t i = 0;
    unsigned long int *uuid_p;
    unsigned long int uuid;
    char * short_id = 0;
    char buf[2];
    char * lastEdgeMask = 0;
    unsigned char xdata
    uuid_bytes[3], *bpos = uuid_bytes;
    char * xdata
    uart_buf[10];
    char * xdata
    uuid_str[8], *pos = uuid_str;
    uint8_t xdata
    receivedData[RECEIVED_DATA_ARRAY_SIZE] = {'\0'};

    uuid_p = 0xfc;
    uuid = *uuid_p;

    //call
    enter_DefaultMode_from_RESET();

    IE_EA = 1;
    XBR2 |= 0x40; //set crossbar regs.

    UART_Begin(115200);
    LED_main(0x44, 0x44, 0x33);

    //Parse chip UUID into long ID (4 bytes)
    sprintf(uuid_str, "%lX", uuid);
    buf[0] = *pos;
    if (buf[0] == 48) buf[0] = 49;
    pos++;
    buf[1] = *pos;
    pos++;
    *bpos = strtol(buf, NULL, 16);
    bpos++;
    buf[0] = *pos;
    if (buf[0] == 48) buf[0] = 49;
    pos++;
    buf[1] = *pos;
    pos++;
    *bpos = strtol(buf, NULL, 16);
    bpos++;
    buf[0] = *pos;
    if (buf[0] == 48) buf[0] = 49;
    pos++;
    buf[1] = *pos;
    pos++;
    *bpos = strtol(buf, NULL, 16);
    bpos++;
    buf[0] = *pos;
    if (buf[0] < 49) buf[0] = 50;
    pos++;
    buf[1] = *pos;
    pos++;
    *bpos = strtol(buf, NULL, 16);

    //Clear detect pins
    DET_A = 0;
    DET_B = 0;
    DET_C = 0;

    while (1) {
        if (UART_Available(false)) {
            receivedData[i] = UART_ReadByte();
            if (receivedData[i] == '\n') {

                //Assign short ID by long ID (0x01)
                if (receivedData[0] == 0x01) {
                    //Is this tile the intended recipient?
                    if (receivedData[2] == uuid_bytes[0] && receivedData[3] == uuid_bytes[1] && receivedData[4] == uuid_bytes[2]) {
                        short_id = (char *) receivedData[6];
                        uart_buf[0] = 0x80;
                        uart_buf[1] = 0;
                        uart_buf[2] = 0;
                        uart_buf[3] = '\0'; //null term
                        UART_Send (uart_buf);
                    }
                }

                //Set edge pin (0x02)
                if (receivedData[0] == 0x02) {
                    if (receivedData[1] == short_id || receivedData[1] == 0xFF) {
                        lastEdgeMask = (char *) receivedData[2];
                        if (receivedData[2] & 0x01) {
                            //Turn A on
                            DET_A = 1;
                        } else {
                            DET_A = 0;
                        }
                        if (receivedData[2] & 0x02) {
                            //Turn B on
                            DET_B = 1;
                        } else {
                            DET_B = 0;
                        }
                        if (receivedData[2] & 0x04) {
                            //Turn C on
                            DET_C = 1;
                        } else {
                            DET_C = 0;
                        }
                        uart_buf[0] = 0x80;
                        uart_buf[1] = 0;
                        uart_buf[2] = 0;
                        uart_buf[3] = '\0'; //null term
                        UART_Send (uart_buf);
                    }
                }

                //Read long ID if selected by EDGE pin (0x03)
                if (receivedData[0] == 0x03) {
                    if (lastEdgeMask == 0) { //Don't read tile if pins were commanded to turn on previously.
                        //If any EDGE pin high, return long id.
                        if (DET_A == 1 || DET_B == 1 || DET_C == 1) {
                            uart_buf[0] = 0x83;
                            uart_buf[1] = (char *) uuid_bytes[0];
                            uart_buf[2] = (char *) uuid_bytes[1];
                            uart_buf[3] = (char *) uuid_bytes[2];
                            uart_buf[4] = (char *) uuid_bytes[3];
                            uart_buf[5] = '\0'; //null term
                            UART_Send (uart_buf);
                        }
                    }
                }

                //Set tile color (0x08)
                if (receivedData[0] == 0x08) {
                    if (receivedData[1] == 0xFF || receivedData[1] == short_id) {
                        uart_buf[0] = 0x80;
                        uart_buf[1] = 0;
                        uart_buf[2] = 0;
                        uart_buf[3] = '\0'; //null term
                        UART_Send (uart_buf);
                        LED_main(receivedData[2], receivedData[3], receivedData[4]);
                    }
                }

                //Call for unconfigured tiles (0x09)
                if (receivedData[0] == 0x09) {
                    if (short_id == 0) {
                        uart_buf[0] = 0x80;
                        uart_buf[1] = 0;
                        uart_buf[2] = 0;
                        uart_buf[3] = '\0'; //null term
                        UART_Send (uart_buf);
                    }
                }
                i = 0;
            } else {
                i++;
            }
        }
    }
}
