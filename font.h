#ifndef FONT_H
#define FONT_H

extern int cursorX;
extern int cursorY;
extern uint8_t textColor[2];

extern char characters[16 * 8];

void font_cursor(int, int);
void font_print(const char*);
void font_text_color(int r, int g, int b, int a);

#endif