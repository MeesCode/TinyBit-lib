#include <stdint.h>
#include <string.h>

#include "graphics.h"
#include "memory.h"
#include "font.h"
#include "assets/basic_font.h"

int cursorX = 0;
int cursorY = 0;
const int fontWidth = 4;
const int fontHeight = 6;
uint32_t textColor = 0xffffffff;

char characters[16 * 8] = {
	'?', '"', '%', '\'', '(', ')', '*', '+', ',', '-', '.', '/', '!',  ' ', ' ', ' ',
	'0', '1', '2', '3', '4',  '5', '6', '7', '8', '9', ':', ';', '<', '=',  '>', '?',
	'@', 'a', 'b', 'c',  'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l',  'm', 'n', 'o',
	'p', 'q', 'r', 's',  't', 'u', 'v', 'w', 'x', 'y', 'z', '[', '\\', ']', '^', '_',
	'`', 'A', 'B', 'C',  'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',  'M', 'N', 'O',
	'P', 'Q', 'R', 'S',  'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '{', '|',  '}', ' ', ' ',
	' ', ' ', ' ', ' ',  ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',  ' ', ' ', ' ',
	' ', ' ', ' ', ' ',  ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',  ' ', ' ', ' '
};

void font_text_color(int r, int g, int b, int a) {
	textColor = (r & 0xFF) | ((g & 0xFF) << 8) | ((b & 0xFF) << 16) | ((a & 0xFF) << 24);
}

void font_cursor(int x, int y) {
	cursorX = x;
	cursorY = y;
}

void font_prints(const char* str) {
	const char* ptr = str;
	int location = 0;

	int startX = cursorX;
	uint32_t fillBackup = fillColor;

	while (*ptr) {

		// process newline character
		if (*ptr == '\n') {
			cursorY += fontHeight;
			cursorX = startX;
			ptr++;
			continue;
		}

		// get character location
		location = 0;
		for (int i = 0; i < 16 * 8; i++) {
			if (*ptr == characters[i]) {
				location = i;
				break; // Stop at the first match
			}
		}

		// draw the character using the draw_pixel function
		int charRow = location / 16;
		int charCol = location % 16;

		for (int y = 0; y < fontHeight; y++) {
			for (int x = 0; x < fontWidth; x++) {
				int byteIndex = (charRow * (fontHeight+2)+y) * 16 + charCol;
				int bitPosition = 7 - x;
				uint8_t font_byte = basic_font[byteIndex];
				if ((font_byte >> bitPosition) & 1) {
					fillColor = textColor;
					draw_pixel(cursorX + x, cursorY + y);
				} else {
					fillColor = fillBackup;
					draw_pixel(cursorX + x, cursorY + y);
				}
			}
		}

		cursorX += fontWidth;
		ptr++;
	}

	fillColor = fillBackup;
}