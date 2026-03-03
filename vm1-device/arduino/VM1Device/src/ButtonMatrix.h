#ifndef BUTTONMATRIX_H
#define BUTTONMATRIX_H

#include "DeviceBufferController.h"

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

void initButtonMatrix();
void checkButtonMatrix();
void updateButtonMatrix();
void addButtonEventToBuffer(ButtonEvent buttonEvent);

#endif