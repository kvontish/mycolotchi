#include <M5Unified.h>
#include <entt/entity/registry.hpp>
#include "components.h"
#include "systems.h"

entt::registry registry;

void setup()
{
    M5.begin();
    Serial.begin(115200);

    registry.set<Camera>();

    auto entity = registry.create();
    registry.emplace<Position>(entity, int16_t(10), int16_t(10));
    registry.emplace<Sprite>(entity, uint16_t(32), uint16_t(32), uint16_t(TFT_PURPLE));
}

void loop()
{
    M5.update();
    render(registry);
}
