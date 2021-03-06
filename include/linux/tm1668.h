/*
 * Copyright (C) 2009 STMicroelectronics Limited
 * Author: Pawel Moll <pawel.moll@st.com>
 *
 * May be copied or modified under the terms of the GNU General Public
 * License.  See linux/COPYING for more information.
 */

#ifndef __TM1668_H
#define __TM1668_H

#include <linux/input.h>

struct tm1668_character {
	char character;
	u16 value;
};

struct tm1668_key {
	u32 mask; /* as read from "keys" attribute */
	int code; /* input event code (KEY_*) */
	char *desc;
};

struct tm1668_platform_data {
	/* Wiring information */

	unsigned gpio_dio, gpio_sclk, gpio_stb;
	enum {
		tm1668_config_6_digits_12_segments,
		tm1668_config_7_digits_11_segments,
	} config;

	/* Keyboard */

	int keys_num;
	struct tm1668_key *keys;
	unsigned long keys_poll_period; /* jiffies */

	/* Display control */

	int brightness; /* initial value, 0 (disabled) - 8 (max) */
	int characters_num;
	struct tm1668_character *characters;
	const char *text; /* initial value, if encoding table available */
};

/* The following encoding tables assume 7 segments display,
 * wired in "standard" order:
 *
 *       0
 *     -----
 *   5|     |1
 *    |  6  |
 *     -----
 *   4|     |2
 *    |     |
 *     -----  .7
 *       3
 */

/* Hexadecimal digits (with lowercase letters) */
#define TM1668_7_SEG_HEX_DIGITS \
		{ '0', 0x03f }, \
		{ '1', 0x006 }, \
		{ '2', 0x05b }, \
		{ '3', 0x04f }, \
		{ '4', 0x066 }, \
		{ '5', 0x06d }, \
		{ '6', 0x07d }, \
		{ '7', 0x007 }, \
		{ '8', 0x07f }, \
		{ '9', 0x06f }, \
		{ 'a', 0x077 }, \
		{ 'b', 0x07c }, \
		{ 'c', 0x058 }, \
		{ 'd', 0x05e }, \
		{ 'e', 0x079 }, \
		{ 'f', 0x071 }

/* Hexadecimal digits with dot (uppercase letters & "<SHIFT>+number") */
#define TM1668_7_SEG_HEX_DIGITS_WITH_DOT \
		{ ')', 0x03f + 0x080 }, \
		{ '!', 0x006 + 0x080 }, \
		{ '@', 0x05b + 0x080 }, \
		{ '#', 0x04f + 0x080 }, \
		{ '$', 0x066 + 0x080 }, \
		{ '%', 0x06d + 0x080 }, \
		{ '^', 0x07d + 0x080 }, \
		{ '&', 0x007 + 0x080 }, \
		{ '*', 0x07f + 0x080 }, \
		{ '(', 0x06f + 0x080 }, \
		{ 'A', 0x077 + 0x080 }, \
		{ 'B', 0x07c + 0x080 }, \
		{ 'C', 0x058 + 0x080 }, \
		{ 'D', 0x05e + 0x080 }, \
		{ 'E', 0x079 + 0x080 }, \
		{ 'F', 0x071 + 0x080 }

/* Individual segments:    ~    *
 *                       {   }  *
 * (useful to make         -    *
 * a clock or            [   ]  *
 * a snake ;-)             _  . */
#define TM1668_7_SEG_SEGMENTS \
		{ '~', 0x001 }, \
		{ '}', 0x002 }, \
		{ ']', 0x004 }, \
		{ '_', 0x008 }, \
		{ '[', 0x010 }, \
		{ '{', 0x020 }, \
		{ '-', 0x040 }, \
		{ '.', 0x080 }

/*
 * Basic Latin alphabetic characters, most legible irrespective
 * of upper or lower case.
 * http://en.wikipedia.org/wiki/Seven-segment_display_character_representations
 */
#define TM1668_7_SEG_LETTERS \
		/*       76543210 */ \
		{ 'A', 0b01110111 }, \
		{ 'B', 0b01111100 }, \
		{ 'C', 0b00111001 }, \
		{ 'D', 0b01011110 }, \
		{ 'E', 0b01111001 }, \
		{ 'F', 0b01110001 }, \
		{ 'G', 0b01101111 }, \
		{ 'H', 0b01110110 }, \
		{ 'I', 0b00110000 }, \
		{ 'J', 0b00011110 }, \
		{ 'K', 0b01110101 }, \
		{ 'L', 0b00111000 }, \
		{ 'M', 0b01010101 }, \
		{ 'N', 0b01010100 }, \
		{ 'O', 0b00111111 }, \
		{ 'P', 0b01110011 }, \
		{ 'Q', 0b01100111 }, \
		{ 'R', 0b01010000 }, \
		{ 'S', 0b01101101 }, \
		{ 'T', 0b01111000 }, \
		{ 'U', 0b00011100 }, \
		{ 'V', 0b00111110 }, \
		{ 'W', 0b00011101 }, \
		{ 'X', 0b01110110 }, \
		{ 'Y', 0b01101110 }, \
		{ 'Z', 0b01011011 }
#endif
