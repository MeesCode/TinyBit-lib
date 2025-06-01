
#include <stdbool.h>
#include <stdint.h>

#include "input.h"
#include "tinybit.h"

uint8_t* button_state;
uint8_t prev_button_state = 0;

bool button_down(enum TinyBitButton b){
	return *button_state & (1 << b);
}

void save_button_state(){
	prev_button_state = *button_state;
}

bool input_btn(enum TinyBitButton b) {
	return button_down(b);
}

bool input_btnp(enum TinyBitButton b) {
	return button_down(b) && !(prev_button_state & (1 << b));
}