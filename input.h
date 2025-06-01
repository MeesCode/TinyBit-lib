#ifndef INPUT_H
#define INPUT_H

#include <stdbool.h>

#include <stdint.h>
#include "tinybit.h"

void save_button_state();
bool input_btn(enum TinyBitButton btn);
bool input_btnp(enum TinyBitButton btn);

extern uint8_t* button_state;

#endif
