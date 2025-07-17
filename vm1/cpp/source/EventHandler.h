#pragma once
#include "EventBus.h"

class IEventHandler
{
private:

public:
    ~IEventHandler() = default;

    virtual void handle(Event event) = 0;
};