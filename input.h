#ifndef INPUT_H
#define INPUT_H

#include <stdbool.h>

typedef enum {
	BUTTON_A,
	BUTTON_B,
	BUTTON_UP,
	BUTTON_DOWN,
	BUTTON_LEFT,
	BUTTON_RIGHT,
	BUTTON_START,
	BUTTON_SELECT
} BUTTON;

void save_button_state();
bool input_btn(BUTTON btn);
bool input_btnp(BUTTON btn);

extern uint8_t* button_state;

#endif
