#ifndef FONT_H
#define FONT_H

extern int cursorX;
extern int cursorY;
extern uint16_t textColor;

extern char characters[16 * 8];

// Font function declarations
void font_cursor(int, int);
void font_print(const char*);
void font_text_color(uint16_t color);

#endif