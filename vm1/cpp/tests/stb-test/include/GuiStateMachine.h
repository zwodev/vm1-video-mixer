#pragma once
#include <memory>

class GuiState;
// enum class StateTransition;

/**
 * @class GuiStateMachine
 * @brief Manages the state transitions for a GUI using a state machine pattern.
 *
 * The GuiStateMachine class holds the current state of the GUI and provides
 * methods to change the state, retrieve the current state, and handle input
 * events that may trigger state transitions.
 */
class GuiStateMachine
{
private:
    GuiState *currentState;

public:
    GuiStateMachine();
    void setState(GuiState *newState);
    GuiState *getCurrentState() const;
    void handleInput(const char &key);
};