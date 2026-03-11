#include <M5Unified.h>
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include "components.h"
#include "systems.h"
#include "scene.h"
#include "game_scene.h"

entt::registry registry;

void onButton(const ButtonEvent &e)
{
    if (e.action != ButtonEvent::Action::Pressed)
        return;

    switch (e.button)
    {
        case ButtonEvent::Button::A: M5.Speaker.tone(262, 100); break; // C4
        case ButtonEvent::Button::B: M5.Speaker.tone(330, 100); break; // E4
        case ButtonEvent::Button::C: M5.Speaker.tone(392, 100); break; // G4
    }
}

void setup()
{
    M5.begin();
    Serial.begin(115200);

    registry.set<entt::dispatcher>();
    registry.set<Camera>();
    const auto &camera = registry.ctx<Camera>();

    registry.set<M5Canvas>(&M5.Display);
    registry.ctx<M5Canvas>().createSprite(camera.w, camera.h);

    registry.ctx<entt::dispatcher>().sink<ButtonEvent>().connect<&onButton>();

    registry.set<SceneManager>();
    registry.ctx<SceneManager>().transition(&GameScene::scene);
}

void loop()
{
    M5.update();
    registry.ctx<SceneManager>().update(registry);
    render(registry);
    showDebugOverlay(registry);
    present(registry);
}
