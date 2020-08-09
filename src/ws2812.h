/*
 * ws2812.h
 *
 *  Created on: Aug 4, 2020
 *      Author: acvig
 */

#ifndef WS2812_H_
#define WS2812_H_

void LED__SendZero_(void);
void LED__SendOne_(void);
void LED_Latch(void);
void LED__SendByte_(unsigned char dat);
void LED_main(unsigned char g, unsigned char r, unsigned char b);

#endif /* WS2812_H_ */
