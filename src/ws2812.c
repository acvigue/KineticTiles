/*
 * ws2812.c
 *
 *  Created on: Aug 4, 2020
 *      Author: acvig
 */
#include <SI_EFM8BB1_Register_Enums.h>
#include <math.h>
SI_SBIT(PIX, SFR_P1, 0);

//gamma correction function for more uniform colors.
int gamma(uint8_t i) {
    return (int) (pow((float) i / (float) 255, 2.8) * 255 + 0.5);
    //return i;
}

void LED__SendZero_(void) {
    PIX = 1;
    _nop_();
    _nop_();
    _nop_();
    _nop_();
    _nop_();
    _nop_();
    _nop_();
    _nop_();
    PIX = 0;
    _nop_();
    _nop_();
    _nop_();
    _nop_();
}
void LED__SendOne_(void) {
    _nop_();
    _nop_();
    _nop_();
    _nop_();
    _nop_();
    _nop_();
    _nop_();
    _nop_();
    _nop_();
    _nop_();
    _nop_();
    _nop_();
    _nop_();
    _nop_();
    _nop_();
    PIX = 0;
    //No NoOPs here because the if statement in SendByte takes 6-7 clock cycles (same as 6-7 NoOPs) +- 150ms
    //Saleae doesn't see it as a valid ws2812 signal, but they still light up :/
}
void LED_Latch(void) {
    short a = 10000;
    PIX = 0;
//Each loop should produce 3 instructions: decrement, comparison, and jmp.
//At least 600 instructions are needed for 50us.
    while (a--)
        ;
}
void LED__SendByte_(unsigned char dat) {
    if (dat & 0x80) {
        PIX = 1;
        LED__SendOne_();
    } else
        LED__SendZero_();
    if (dat & 0x40) {
        PIX = 1;
        LED__SendOne_();
    } else
        LED__SendZero_();
    if (dat & 0x20) {
        PIX = 1;
        LED__SendOne_();
    } else
        LED__SendZero_();
    if (dat & 0x10) {
        PIX = 1;
        LED__SendOne_();
    } else
        LED__SendZero_();
    if (dat & 0x08) {
        PIX = 1;
        LED__SendOne_();
    } else
        LED__SendZero_();
    if (dat & 0x04) {
        PIX = 1;
        LED__SendOne_();
    } else
        LED__SendZero_();
    if (dat & 0x02) {
        PIX = 1;
        LED__SendOne_();
    } else
        LED__SendZero_();
    if (dat & 0x01) {
        PIX = 1;
        LED__SendOne_();
    } else
        LED__SendZero_();
}

void LED__SendPixel_(uint8_t g, uint8_t r, uint8_t b) {
    LED__SendByte_(r);
    LED__SendByte_(g);
    LED__SendByte_(b);
}

void LED_main(unsigned char g, unsigned char r, unsigned char b) {
    int i;
    uint8_t nr = gamma(r);
    uint8_t ng = gamma(g);
    uint8_t nb = gamma(b);

    //Send, latch, and send again because first pixel might not light up on the first go round.
    for (i = 0; i < 9; i++) {
        LED__SendByte_(nr);
        LED__SendByte_(ng);
        LED__SendByte_(nb);
    }
    LED_Latch();
    for (i = 0; i < 9; i++) {
        LED__SendByte_(nr);
        LED__SendByte_(ng);
        LED__SendByte_(nb);
    }
    LED_Latch();
}
