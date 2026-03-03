// #include "ProjectionMappingController.h"

// ProjectionMappingController::ProjectionMappingController(Registry& registry, EventBus& eventBus) :
// m_registry(registry),
// m_eventBus(eventBus)
// {
//     subscribeToEvents();
// }

// ProjectionMappingController::~ProjectionMappingController(){}


// void ProjectionMappingController::subscribeToEvents()
// {
//     m_eventBus.subscribe<MappingButtonEvent>([this](const MappingButtonEvent& event){
//         // printf("Vertex ID: %d dX: %f dY: %f\n", event.vertexId, event.x, event.y);
//         m_registry.planeSettings().coords.at(event.vertexId).x += event.x;
//         m_registry.planeSettings().coords.at(event.vertexId).y += event.y;
//     });
// }