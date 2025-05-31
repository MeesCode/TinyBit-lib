
#include <stdbool.h>
#include <stdint.h>

#include "input.h"

uint8_t* button_state;
uint8_t prev_button_state = 0;

BUTTON button_down(BUTTON b){
	return *button_state & (1 << b);
}

void save_button_state(){
	prev_button_state = *button_state;
}

bool input_btn(BUTTON b) {
	return button_down(b);
}

bool input_btnp(BUTTON b) {
	return button_down(b) && !(prev_button_state & (1 << b));
}