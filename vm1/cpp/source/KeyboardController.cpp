#include "KeyboardController.h"

#include "stdio.h"

KeyboardController::KeyboardController(Registry &registry, EventBus &eventBus) : 
    m_registry(registry), m_eventBus(eventBus)
{
}

KeyboardController::~KeyboardController()
{
}

