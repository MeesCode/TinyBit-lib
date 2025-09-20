
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "input.h"
#include "tinybit.h"

static bool* button_state;
bool prev_button_state[TB_BUTTON_COUNT] = {0};

void init_input(bool* button_state_ptr) {
	button_state = button_state_ptr;
}

// Check if a button is currently being pressed
bool button_down(enum TinyBitButton b){
	return button_state[b];
}

// Save the current button state for next frame comparison
void save_button_state(){
	memcpy(prev_button_state, button_state, sizeof(prev_button_state));
}

// Check if a button is currently being held down
bool input_btn(enum TinyBitButton b) {
	return button_down(b);
}

// Check if a button was just pressed this frame (not held from previous frame)
bool input_btnp(enum TinyBitButton b) {
	return button_down(b) && !prev_button_state[b];
}