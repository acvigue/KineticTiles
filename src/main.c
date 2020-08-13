/*
 * RGB Tile v1.4A code.
 * Copyright 2020
 * Written on: Aug 5, 2020
 * By: Aiden Vigue (acvig)
 */

#include <SI_EFM8BB1_Register_Enums.h>
#include <stdio.h>
#include <stdlib.h>
#include "InitDevice.h"
#include "ws2812.h"

SI_SBIT(DET_C, SFR_P0, 6);
SI_SBIT(DET_A, SFR_P0, 1);
SI_SBIT(DET_B, SFR_P1, 5);

void SiLabs_Startup(void) {

}

char WriteChar(char c) {
    SBUF0 = c;
    SCON0_TI = 0;
    return c;
}

char ReadChar() {
    while (!SCON0_RI)
        ;
    SCON0_RI = 0;
    return SBUF0;
}

void UART_Send(char* *s) {
    int i;
    for (i = 0; i < 10; i++) {
        WriteChar(s[i]);
        ReadChar();
    }
}

void main(void) {
    uint8_t i = 0;
    unsigned long int *uuid_p;
    unsigned long int uuid;
    char * short_id = 0;
    char buf[2];
    int lastEdgeMask = 0;
    unsigned char xdata
    uuid_bytes[3], *bpos = uuid_bytes;
    char * xdata
    uart_buf[10];
    char * xdata
    uuid_str[8], *pos = uuid_str;
    uint8_t xdata
    receivedData[10] = {'\0'};

    uuid_p = 0xfc;
    uuid = *uuid_p;

    //call
    enter_DefaultMode_from_RESET();

    IE_EA = 1;
    XBR2 |= 0x40; //set crossbar regs.

    LED_main(100, 100, 100);

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

    //Set all edge pins to Hi-Z
    DET_A = 1;
    DET_B = 1;
    DET_C = 1;
    while (1) {
        receivedData[i] = ReadChar();

        if (receivedData[i] == '\n') {

            //Assign short ID by long ID (0x01)
            if (receivedData[0] == 0x01) {
                //Is this tile the intended recipient?
                if (receivedData[2] == uuid_bytes[0] && receivedData[3] == uuid_bytes[1] && receivedData[4] == uuid_bytes[2]) {
                    short_id = (char *) receivedData[6];
                    uart_buf[0] = 0x80;
                    uart_buf[1] = 0x01;
                    uart_buf[2] = 0xA0;
                    UART_Send (uart_buf);
                }
            }

            //Set edge pin (0x02)
            //Pins pulled to ground will influence neighbor if their pin is floating.
            if (receivedData[0] == 0x02) {
                if (receivedData[1] == short_id || receivedData[1] == 0xFF) {
                    lastEdgeMask = (int *) receivedData[2];
                    if (receivedData[2] & 0x01) {
                        DET_A = 0; //drive A to gnd
                    } else {
                        DET_A = 1; //Hi-Z
                    }
                    if (receivedData[2] & 0x02) {
                        DET_B = 0; //drive B to gnd
                    } else {
                        DET_B = 1; //Hi-Z
                    }
                    if (receivedData[2] & 0x04) {
                        DET_C = 0; //drive C to gnd
                    } else {
                        DET_C = 1; //Hi-Z
                    }
                    uart_buf[0] = 0x80;
                    uart_buf[1] = 0x02;
                    uart_buf[2] = 0xA0;
                    UART_Send (uart_buf);
                }
            }

            //Read long ID if selected by EDGE pin (0x03)
            if (receivedData[0] == 0x03) {
                if (lastEdgeMask == 0 && short_id == 0) {
                    //If any edge pin low, return long id.
                    if (DET_A == 0 || DET_B == 0 || DET_C == 0) {
                        uart_buf[0] = 0x80;
                        uart_buf[1] = (char *) uuid_bytes[0];
                        uart_buf[2] = (char *) uuid_bytes[1];
                        uart_buf[3] = (char *) uuid_bytes[2];
                        uart_buf[4] = (char *) uuid_bytes[3];
                        uart_buf[5] = 0xA0;
                        UART_Send (uart_buf);
                    }
                }
            }

            //Set tile color (0x08)
            if (receivedData[0] == 0x08) {
                if (receivedData[1] == 0xFF || receivedData[1] == short_id) {
                    uart_buf[0] = 0x80;
                    uart_buf[1] = 0x08;
                    uart_buf[2] = 0xA0;
                    LED_main(receivedData[2], receivedData[3], receivedData[4]);
                    UART_Send (uart_buf);
                }
            }

            //Call for unconfigured tiles (0x09)
            if (receivedData[0] == 0x09) {
                if (short_id == 0) {
                    uart_buf[0] = 0x80;
                    uart_buf[1] = 0x09;
                    uart_buf[2] = 0xA0;
                    UART_Send (uart_buf);
                }
            }

            //Halt
            if (receivedData[0] == 0xFF) {
                //__asm MOV 0A7h,#0
                //__asm MOV 00,#0A5h
                //__asm MOV 0EFh,#010h
            }
        i = 0;
    } else {
        i++;
    }
}
}
