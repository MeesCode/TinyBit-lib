#include <stdint.h>
#include <string.h>

#include "graphics.h"
#include "memory.h"
#include "font.h"
#include "assets/basic_font.h"
#include "tinybit.h"

int cursorX = 0;
int cursorY = 0;
const int fontWidth = 4;
const int fontHeight = 6;
uint16_t textColor = 0;

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

// Set the text color for font rendering
void font_text_color(uint16_t color) {
	textColor = color;
}

// Set the cursor position for text rendering
void font_cursor(int x, int y) {
	cursorX = x;
	cursorY = y;
}

// Print text string at current cursor position using bitmap font
void font_print(const char* str) {
	const char* ptr = str;
	int location = 0;

	int startX = cursorX;

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

		// draw the character using direct pixel blending
		int charRow = location / 16;
		int charCol = location % 16;

		for (int y = 0; y < fontHeight; y++) {
			for (int x = 0; x < fontWidth; x++) {
				int px = cursorX + x;
				int py = cursorY + y;
				
				if (px >= 0 && px < TB_SCREEN_WIDTH && py >= 0 && py < TB_SCREEN_HEIGHT) {
					int byteIndex = (charRow * (fontHeight+2)+y) * 16 + charCol;
					
					// Bounds check for font array access
					if (byteIndex >= 0 && byteIndex < sizeof(basic_font)) {
						int bitPosition = 7 - x;
						uint8_t font_byte = basic_font[byteIndex];
						uint16_t* pixel = &tinybit_memory->display[py * TB_SCREEN_WIDTH + px];

						if ((font_byte >> bitPosition) & 1) {
							blend(pixel, textColor);
						}
						else {
							blend(pixel, fillColor);
						}
					}
				}
			}
		}

		cursorX += fontWidth;
		ptr++;
	}

}