#include "KeyboardController.h"

#include <iostream>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

void KeyboardController::setRawMode(bool enable)
{
    static struct termios oldt, newt;
    if (enable)
    {
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO); // disable canonical mode and echo
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK); // non-blocking input

        std::cout << "Raw mode enabled. Press 'q' to quit.\n";
    }
    else
    {
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt); // restore
        fcntl(STDIN_FILENO, F_SETFL, 0);         // blocking input
        std::cout << "Raw mode disabled.\n";
    }
}

KeyboardController::KeyboardController()
{
    setRawMode(true);
}

KeyboardController::~KeyboardController()
{
    setRawMode(false);
}

// char KeyboardController::getKeyPress()
// {
//     char ch = 0;
//     if (read(STDIN_FILENO, &ch, 1) == 1)
//     {
//         return ch;
//     }
//     return 0;
// }

char KeyboardController::getKeyPress()
{
    char ch = 0;
    if (read(STDIN_FILENO, &ch, 1) == 1)
    {
        if (ch == 27)
        {
            // Escape sequence - read two more bytes
            char seq[2];
            if (read(STDIN_FILENO, &seq[0], 1) != 1)
                return 27;
            if (read(STDIN_FILENO, &seq[1], 1) != 1)
                return 27;

            if (seq[0] == '[')
            {
                switch (seq[1])
                {
                case 'A':
                    return '+'; // Up
                case 'B':
                    return '-'; // Down
                case 'C':
                    return '\n'; // Right
                case 'D':
                    return 'b'; // Left
                }
            }
            return 27; // Unknown escape sequence
        }
        else
        {
            return ch;
        }
    }
    return 0;
}
