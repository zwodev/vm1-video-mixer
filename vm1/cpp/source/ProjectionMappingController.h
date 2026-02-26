#pragma once

#include "Registry.h"
#include "EventBus.h"

class ProjectionMappingController
{
public:
    ProjectionMappingController(Registry& registry, EventBus& eventBus);
    ~ProjectionMappingController();

private:
    void subscribeToEvents();
    void updatePlaneVertices();

    Registry& m_registry;
    EventBus& m_eventBus;
};