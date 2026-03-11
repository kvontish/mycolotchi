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
    const auto &camera = registry.ctx<Camera>();

    registry.set<M5Canvas>(&M5.Display);
    registry.ctx<M5Canvas>().createSprite(camera.w, camera.h);

    auto purpleBox = registry.create();
    registry.emplace<Position>(purpleBox, int16_t(10), int16_t(10), 0.5f);
    registry.emplace<Sprite>(purpleBox, uint16_t(32), uint16_t(32), uint16_t(TFT_PURPLE));

    auto blueBox = registry.create();
    registry.emplace<Position>(blueBox, int16_t(50), int16_t(50));
    registry.emplace<Sprite>(blueBox, uint16_t(32), uint16_t(32), uint16_t(TFT_BLUE));
}

void loop()
{
    M5.update();
    debugPanCamera(registry);

    // rendering/presentation systems
    render(registry);
    debugFps(registry);
    present(registry);
}
