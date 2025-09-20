#ifndef INPUT_H
#define INPUT_H

#include <stdbool.h>

#include <stdint.h>
#include "tinybit.h"

// Input function declarations
void init_input(bool* button_state_ptr);
void save_button_state();
bool input_btn(enum TinyBitButton btn);
bool input_btnp(enum TinyBitButton btn);

#endif
