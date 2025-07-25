#ifndef KEYBOARDMATRIX_H
#define KEYBOARDMATRIX_H

#include "Keyboard.h"

// input rows pins
#define ROW1 6
#define ROW2 5
#define ROW3 4

// output colums pins
#define COL1 7
#define COL2 8
#define COL3 9
#define COL4 10
#define COL5 11
#define COL6 12
#define COL7 13
#define COL8 14
#define COL9 15

#define PRESSED 0
#define RELEASED 1

void init_keyboard();
void check_keyboard_matrix();
void update_keyboard();
void set_keyboard_buffer(char* buffer, uint8_t size);
void clear_keyboard_buffer();

#endif