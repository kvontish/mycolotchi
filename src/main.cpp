#include "clock_scene.h"
#include "components.h"
#include "game_over_scene.h"
#include "game_scene.h"
#include "home_scene.h"
#include "menu_scene.h"
#include "scene.h"
#include "status_scene.h"
#include "systems.h"
#include "title_scene.h"
#include "walk_scene.h"
#include <M5Unified.h>
#include <SD.h>
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>

entt::registry registry;

volatile ButtonState gButtonState[3]{ButtonState::None, ButtonState::None, ButtonState::None};

static constexpr uint32_t kLongPressMs = 500;

void inputTask(void *) {
    static uint32_t pressStartMs[3]{0, 0, 0};
    m5::Button_Class *btns[3]{&M5.BtnA, &M5.BtnB, &M5.BtnC};

    for (;;) {
        M5.update();
        for (uint8_t i = 0; i < 3; i++) {
            if (btns[i]->wasPressed())
                pressStartMs[i] = millis();
            if (btns[i]->wasReleased()) {
                gButtonState[i] =
                    millis() - pressStartMs[i] >= kLongPressMs ? ButtonState::LongPressed : ButtonState::Pressed;
            }
        }
        detectSteps();
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void setup() {
    M5.begin();
    Serial.begin(115200);
    SD.begin(4); // M5Core2 SD CS pin

    registry.set<entt::dispatcher>();
    registry.set<Camera>();
    const auto &camera = registry.ctx<Camera>();

    registry.set<M5Canvas>(&M5.Display);
    registry.ctx<M5Canvas>().createSprite(camera.w, camera.h);

    registry.set<Clock>();
    tickClock(registry); // populate from RTC before first frame

    registry.set<Pet>();

    auto &map = registry.set<GameMap>();
    map.homeScene = &homeScene;
    map.titleScene = &titleScene;
    map.gameScene = &gameScene;
    map.clockScene = &clockScene;
    map.menuScene = &menuScene;
    map.walkScene = &walkScene;
    map.gameOverView = &gameOverView;
    map.statusView = &statusView;

    registry.set<SceneManager>();
    registry.ctx<SceneManager>().transition(&homeScene);

    xTaskCreatePinnedToCore(inputTask, "input", 2048, nullptr, 1, nullptr, 0);
}

void loop() {
    updateDisplayState();
    decayPetStats(registry);
    if (isDisplayDimmed())
        return;
    tickClock(registry);
    registry.ctx<SceneManager>().update(registry);
    if (auto *view = registry.ctx<SceneManager>().activeView())
        render(registry, view);
    else
        render(registry);
    // renderHitboxes(registry);
    showDebugOverlay(registry);
    present(registry);
}
