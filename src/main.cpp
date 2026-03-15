#include "components.h"
#include "game_over_scene.h"
#include "game_scene.h"
#include "scene.h"
#include "systems.h"
#include "title_scene.h"
#include <M5Unified.h>
#include <SD.h>
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>

entt::registry registry;

volatile bool gBtnPressed[3]{false, false, false};

void inputTask(void *) {
    for (;;) {
        M5.update();
        if (M5.BtnA.wasPressed())
            gBtnPressed[0] = true;
        if (M5.BtnB.wasPressed())
            gBtnPressed[1] = true;
        if (M5.BtnC.wasPressed())
            gBtnPressed[2] = true;
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void setup() {
    M5.begin();
    Serial.begin(115200);
    SD.begin(4); // M5Core2 SD CS pin

    titleScene.nextScene = &gameScene;
    gameOverScene.nextScene = &gameScene;

    xTaskCreatePinnedToCore(inputTask, "input", 2048, nullptr, 1, nullptr, 0);

    registry.set<entt::dispatcher>();
    registry.set<Camera>();
    const auto &camera = registry.ctx<Camera>();

    registry.set<M5Canvas>(&M5.Display);
    registry.ctx<M5Canvas>().createSprite(camera.w, camera.h);

    registry.set<SceneManager>();
    registry.ctx<SceneManager>().transition(&titleScene);
}

void loop() {
    sleepIfInactive();
    registry.ctx<SceneManager>().update(registry);
    render(registry);
    //renderHitboxes(registry);
    showDebugOverlay(registry);
    present(registry);
}
