#ifndef INPUT_H
#define INPUT_H

#include <stdbool.h>

void save_button_state();
bool input_btn(enum TinyBitButton btn);
bool input_btnp(enum TinyBitButton btn);

extern uint8_t* button_state;

#endif
