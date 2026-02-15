
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "input.h"
#include "tinybit.h"
#include "memory.h"

bool prev_button_state[TB_BUTTON_COUNT] = {0};

// Check if a button is currently being pressed
bool button_down(enum TinyBitButton b){
	return tinybit_memory->button_input[b];
}

// Save the current button state for next frame comparison
void save_button_state(){
	memcpy(prev_button_state, tinybit_memory->button_input, sizeof(prev_button_state));
}

// Check if a button is currently being held down
bool input_btn(enum TinyBitButton b) {
	return button_down(b);
}

// Check if a button was just pressed this frame (not held from previous frame)
bool input_btnp(enum TinyBitButton b) {
	return button_down(b) && !prev_button_state[b];
}