#include <iostream>
#include "GuiStateMachine.h"
#include "GuiStates.h"

GuiStateMachine::GuiStateMachine() : currentState(nullptr) {}

void GuiStateMachine::setState(GuiState *newState)
{
    currentState = newState;
}

GuiState *GuiStateMachine::getCurrentState() const
{
    return currentState;
}

void GuiStateMachine::handleInput(const char &key)
{
    if (currentState)
    {
        currentState->handleInput(key);
    }
}
