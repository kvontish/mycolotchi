#include <M5Unified.h>
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include "components.h"
#include "systems.h"
#include "scene.h"
#include "title_scene.h"

entt::registry registry;

void setup()
{
    M5.begin();
    Serial.begin(115200);

    registry.set<entt::dispatcher>();
    registry.set<Camera>();
    const auto &camera = registry.ctx<Camera>();

    registry.set<M5Canvas>(&M5.Display);
    registry.ctx<M5Canvas>().createSprite(camera.w, camera.h);

    registry.set<SceneManager>();
    registry.ctx<SceneManager>().transition(&titleScene);
}

void loop()
{
    M5.update();
    registry.ctx<SceneManager>().update(registry);
    render(registry);
    showDebugOverlay(registry);
    present(registry);
}
