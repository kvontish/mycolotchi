#include <M5Unified.h>
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include "components.h"
#include "systems.h"

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

    auto &dispatcher = registry.ctx<entt::dispatcher>();
    dispatcher.sink<ButtonEvent>().connect<&onButton>();

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
    pollInput(registry);
    debugPanCamera(registry);

    // rendering/presentation systems
    render(registry);
    debugFps(registry);
    present(registry);
}
