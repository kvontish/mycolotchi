#pragma once

#include "components.h"
#include "events.h"
#include "scene.h"
#include <M5Unified.h>
#include <algorithm>
#include <entt/entity/registry.hpp>
#include <entt/signal/dispatcher.hpp>
#include <esp_sleep.h>
#include <vector>

// =============================================================================
// Input
// =============================================================================

static uint32_t sLastActivityMs = 0;
static bool sDisplayDimmed = false;
static volatile uint32_t sDetectedSteps = 0;
static volatile bool sStepCountingActive = false;

inline void pollInput(entt::registry &registry) {
    auto &dispatcher = registry.ctx<entt::dispatcher>();

    for (uint8_t i = 0; i < 3; i++) {
        if (gButtonState[i] != ButtonState::None) {
            ButtonState action = gButtonState[i];
            gButtonState[i] = ButtonState::None;
            sLastActivityMs = millis();
            dispatcher.enqueue<ButtonEvent>({ButtonEvent::Button(i), action});
        }
    }

    dispatcher.update<ButtonEvent>();
}

// FT6336 touch controller INT pin — fires low on any touch, usable as ext0 wake source
static constexpr gpio_num_t kTouchIntPin = GPIO_NUM_39;
static constexpr uint32_t kSleepTimeoutMs = 30000;
static uint8_t sSavedBrightness = 128;

inline bool isDisplayDimmed() { return sDisplayDimmed; }

// Walk mode: dim display and reduce CPU — Core 0 keeps running for pedometer.
// loop() returns early while dimmed; wakeup is detected via button activity.
inline void enterDimSleep() {
    if (sDisplayDimmed)
        return;
    sSavedBrightness = M5.Display.getBrightness();
    M5.Display.setBrightness(0);
    M5.Display.sleep();
    setCpuFrequencyMhz(80);
    sDisplayDimmed = true;
}

inline void discardButtonStates() {
    for (uint8_t i = 0; i < 3; i++)
        gButtonState[i] = ButtonState::None;
}

inline void exitDimSleep() {
    if (!sDisplayDimmed)
        return;
    setCpuFrequencyMhz(240);
    M5.Display.wakeup();
    M5.Display.setBrightness(sSavedBrightness);
    sDisplayDimmed = false;
    discardButtonStates(); // throw away waking button presses
}

// Idle mode: full light sleep — both cores suspended until touch wakes the device.
// Blocks until wakeup, then restores display and discards the wake press.
inline void enterLightSleep() {
    // Dim the display
    sSavedBrightness = M5.Display.getBrightness();
    M5.Display.setBrightness(0);
    M5.Display.sleep();

    // Put the device to sleep
    esp_sleep_enable_ext0_wakeup(kTouchIntPin, 0);
    esp_light_sleep_start();

    // On wakeup, logic starts from here
    sLastActivityMs = millis();
    M5.Display.wakeup();
    M5.Display.setBrightness(sSavedBrightness);
    discardButtonStates(); // throw away waking button presses
}

inline void updateDisplayState() {
    // Get the last activity
    for (uint8_t i = 0; i < 3; i++) {
        if (gButtonState[i] != ButtonState::None)
            sLastActivityMs = millis();
    }
    bool inactive = millis() - sLastActivityMs >= kSleepTimeoutMs;

    if (inactive) {
        // Counting steps requires dim (pseudo) sleep
        if (sStepCountingActive)
            enterDimSleep();
        // Otherwise use true device light sleep
        else
            enterLightSleep();
    } else {
        exitDimSleep();
    }
}

// =============================================================================
// Time
// =============================================================================

inline uint32_t toUnixTime(uint16_t y, uint8_t m, uint8_t d, uint8_t h, uint8_t mi, uint8_t s) {
    static const uint16_t kDaysBeforeMonth[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
    uint16_t y0 = y - 1;
    uint32_t days = (y - 1970) * 365UL + (y0 / 4 - 1969 / 4) - (y0 / 100 - 1969 / 100) + (y0 / 400 - 1969 / 400) +
                    kDaysBeforeMonth[m - 1];
    if (m > 2 && (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0)))
        days++;
    days += d - 1;
    return days * 86400UL + h * 3600UL + mi * 60UL + s;
}

inline void tickClock(entt::registry &registry) {
    auto &clock = registry.ctx<Clock>();
    auto t = M5.Rtc.getTime();
    auto d = M5.Rtc.getDate();
    clock.hours = t.hours;
    clock.minutes = t.minutes;
    clock.seconds = t.seconds;
    clock.year = d.year;
    clock.month = d.month;
    clock.day = d.date;
    clock.timestamp = toUnixTime(clock.year, clock.month, clock.day, clock.hours, clock.minutes, clock.seconds);
}

// =============================================================================
// Pedometer
// =============================================================================

// Pokéwalker-style pedometer:
//   1. Gravity removal via very-slow LP (~0.05 Hz) → isolates dynamic acceleration
//   2. Bandpass LP (~4 Hz) on gravity-removed signal → walking band (1-3 Hz)
//   3. Downward zero-crossing of bandpass → one candidate step per crossing
//   4. Rhythm gate: buffer the last N inter-crossing intervals; open gate when
//      coefficient-of-variation² drops below threshold (consistent cadence)
//   5. Retroactive counting: when gate opens flush all pending crossings as steps;
//      while open, count each crossing; close on rhythm break or long pause

static constexpr float kGravityAlpha = 0.003f;   // ~0.05 Hz LP for gravity estimate
static constexpr float kBandpassAlpha = 0.22f;   // ~4 Hz LP for walking band
static constexpr uint32_t kMinIntervalMs = 250;  // fastest plausible step (4 Hz)
static constexpr uint32_t kMaxIntervalMs = 2000; // slowest plausible step (0.5 Hz)
static constexpr uint8_t kRhythmBufSize = 3;     // intervals to average for gate
static constexpr float kMaxRhythmCV2 = 0.20f;    // max CV² to keep gate open

inline void detectSteps() {
    if (!sStepCountingActive)
        return;

    static float gravity = 1.0f;
    static float bandpass = 0.0f;
    static float prevBandpass = 0.0f;
    static uint32_t lastCrossMs = 0;
    static uint32_t intervals[kRhythmBufSize]{0, 0, 0};
    static uint8_t intervalIdx = 0;
    static uint8_t intervalFill = 0; // how many intervals are populated
    static uint8_t pendingSteps = 0; // crossings waiting for rhythm confirmation
    static bool gateOpen = false;

    float ax, ay, az;
    M5.Imu.getAccel(&ax, &ay, &az);
    float mag = sqrtf(ax * ax + ay * ay + az * az);

    // Step 1: gravity estimate (very slow LP)
    gravity = gravity * (1.0f - kGravityAlpha) + mag * kGravityAlpha;

    // Step 2: bandpass (LP on gravity-removed signal)
    float dynamic = mag - gravity;
    prevBandpass = bandpass;
    bandpass = bandpass * (1.0f - kBandpassAlpha) + dynamic * kBandpassAlpha;

    // Step 3: downward zero-crossing detection
    if (prevBandpass >= 0.0f && bandpass < 0.0f) {
        uint32_t now = millis();
        uint32_t interval = now - lastCrossMs;

        if (lastCrossMs == 0 || interval < kMinIntervalMs || interval > kMaxIntervalMs) {
            // Invalid crossing — reset rhythm state
            gateOpen = false;
            pendingSteps = 0;
            intervalFill = 0;
        } else {
            // Step 4: update rhythm buffer
            intervals[intervalIdx] = interval;
            intervalIdx = (intervalIdx + 1) % kRhythmBufSize;
            if (intervalFill < kRhythmBufSize)
                intervalFill++;

            if (intervalFill == kRhythmBufSize) {
                // Compute mean and CV² over buffered intervals
                float sum = 0.0f;
                for (uint8_t i = 0; i < kRhythmBufSize; i++)
                    sum += (float)intervals[i];
                float mean = sum / kRhythmBufSize;
                float var = 0.0f;
                for (uint8_t i = 0; i < kRhythmBufSize; i++) {
                    float d = (float)intervals[i] - mean;
                    var += d * d;
                }
                var /= kRhythmBufSize;
                float cv2 = (mean > 0.0f) ? var / (mean * mean) : 1.0f;

                if (cv2 <= kMaxRhythmCV2) {
                    if (!gateOpen) {
                        // Step 5: gate just opened — count all pending crossings + this one
                        sDetectedSteps += pendingSteps + 1;
                        pendingSteps = 0;
                        gateOpen = true;
                    } else {
                        sDetectedSteps++;
                    }
                } else {
                    // Rhythm broke — close gate, discard pending
                    gateOpen = false;
                    pendingSteps = 0;
                }
            } else {
                // Not enough history yet — accumulate as pending
                pendingSteps++;
            }
        }

        lastCrossMs = now;
    } else if (gateOpen && lastCrossMs > 0 && millis() - lastCrossMs > kMaxIntervalMs) {
        // Stepped too long ago — close gate
        gateOpen = false;
        pendingSteps = 0;
    }
}

// =============================================================================
// Pet
// =============================================================================

static constexpr uint32_t kHungerDecayIntervalMs = 5UL * 60UL * 1000UL;     // -1 hunger per 5 min
static constexpr uint32_t kHappinessDecayIntervalMs = 10UL * 60UL * 1000UL; // -1 happiness per 10 min

inline void decayPetStats(entt::registry &registry) {
    static uint32_t lastHungerDecayMs = 0;
    static uint32_t lastHappinessDecayMs = 0;

    uint32_t now = millis();
    auto &pet = registry.ctx<Pet>();

    if (now - lastHungerDecayMs >= kHungerDecayIntervalMs) {
        if (pet.hunger > 0)
            pet.hunger--;
        lastHungerDecayMs = now;
    }

    if (now - lastHappinessDecayMs >= kHappinessDecayIntervalMs) {
        if (pet.happiness > 0)
            pet.happiness--;
        lastHappinessDecayMs = now;
    }
}

// =============================================================================
// Simulation
// =============================================================================

// --- Physics ---

inline void physics(entt::registry &registry) {
    const int16_t kGravityAccel = 1;

    auto view = registry.view<Position, Velocity>();
    view.each([&registry, kGravityAccel](entt::entity entity, Position &pos, Velocity &vel) {
        if (registry.all_of<Gravity>(entity))
            vel.y += kGravityAccel;
        pos.x += vel.x;
        pos.y += vel.y;
    });
}

inline void groundCheck(entt::registry &registry) {
    int16_t groundY = INT16_MAX;
    registry.view<Solid, Position>().each([&registry, &groundY](entt::entity e, const Position &pos) {
        const Hitbox *hb = registry.try_get<Hitbox>(e);
        int16_t y = pos.y + (hb ? hb->oy : int8_t(0));
        if (y < groundY)
            groundY = y;
    });

    registry.view<Player, Position, Velocity, Sprite>().each(
        [&registry, groundY](entt::entity entity, Position &pos, Velocity &vel, const Sprite &sprite) {
        if (vel.y >= 0 && pos.y + (int16_t)sprite.h >= groundY) {
            pos.y = groundY - (int16_t)sprite.h;
            vel.y = 0;
            registry.emplace_or_replace<Grounded>(entity);
        } else if (registry.all_of<Grounded>(entity)) {
            registry.remove<Grounded>(entity);
        }
    });
}

// --- Camera ---

inline void moveCamera(entt::registry &registry) {
    auto &camera = registry.ctx<Camera>();
    auto view = registry.view<CameraTarget, Position>();
    view.each([&camera](const Position &pos) {
        camera.x = pos.x;
        camera.y = pos.y;
    });
}

// --- Animation ---

inline void animateSprites(entt::registry &registry) {
    uint32_t now = millis();
    registry.view<AnimationState, Sprite>().each([now](AnimationState &state, Sprite &sprite) {
        const Animation &anim = state.set->animations[state.currentAnimation];
        if (now - state.lastFrameMs >= anim.frameDurationMs) {
            if (anim.loop || state.currentFrame + 1 < anim.frameCount)
                state.currentFrame = (state.currentFrame + 1) % anim.frameCount;
            state.lastFrameMs = now;
        }
        sprite.data = anim.frames[state.currentFrame];
    });
}

// =============================================================================
// Rendering
// =============================================================================

inline void drawSprite(M5Canvas &canvas,
                       const Sprite &sprite,
                       int16_t x,
                       int16_t y,
                       int8_t scaleX = 1,
                       int8_t scaleY = 1) {
    if (sprite.color != TFT_TRANSPARENT)
        canvas.fillRect(x, y, sprite.w, sprite.h, sprite.color);
    if (!sprite.data)
        return;
    if (scaleX == 1 && scaleY == 1) {
        sprite.data->pushSprite(&canvas, x, y, TFT_TRANSPARENT);
    } else {
        // pushRotateZoom takes the destination center point; use abs(scale) so negative (mirror) scales
        // don't shift the sprite off its bounding box
        float cx = x + sprite.w * (scaleX < 0 ? -scaleX : scaleX) * 0.5f;
        float cy = y + sprite.h * (scaleY < 0 ? -scaleY : scaleY) * 0.5f;
        sprite.data->pushRotateZoom(&canvas, cx, cy, 0.0f, (float)scaleX, (float)scaleY, (uint16_t)TFT_TRANSPARENT);
    }
}

inline void renderTiled(const Tiled &tiled,
                        const Sprite &sprite,
                        int16_t baseX,
                        int16_t baseY,
                        uint16_t camW,
                        uint16_t camH,
                        M5Canvas &canvas) {
    int16_t startX = tiled.x ? (baseX % (int16_t)sprite.w + sprite.w) % sprite.w - sprite.w : baseX;
    int16_t startY = tiled.y ? (baseY % (int16_t)sprite.h + sprite.h) % sprite.h - sprite.h : baseY;
    int16_t endX = tiled.x ? camW : baseX + sprite.w;
    int16_t endY = tiled.y ? camH : baseY + sprite.h;

    for (int16_t ty = startY; ty < endY; ty += sprite.h)
        for (int16_t tx = startX; tx < endX; tx += sprite.w)
            drawSprite(canvas, sprite, tx, ty); // tiled backgrounds are never scaled
}

inline void drawEntity(entt::registry &registry,
                       const Camera &camera,
                       M5Canvas &canvas,
                       entt::entity entity,
                       const Position &pos,
                       const Sprite &sprite) {
    int16_t baseX = pos.x - (int16_t)(camera.x * pos.parallax);
    int16_t baseY = pos.y - (int16_t)(camera.y * pos.parallax);

    const Tiled *tiled = registry.try_get<Tiled>(entity);
    if (tiled) {
        renderTiled(*tiled, sprite, baseX, baseY, camera.w, camera.h, canvas);
        return;
    }

    if (baseX + sprite.w <= 0 || baseX >= camera.w)
        return;
    if (baseY + sprite.h <= 0 || baseY >= camera.h)
        return;

    drawSprite(canvas, sprite, baseX, baseY, pos.scaleX, pos.scaleY);
}

inline bool ownedBy(entt::registry &registry, entt::entity e, View *viewFilter) {
    const ViewOwner *owner = registry.try_get<ViewOwner>(e);
    return (owner ? owner->view : nullptr) == viewFilter;
}

// viewFilter == nullptr → render only scene entities (no ViewOwner)
// viewFilter != nullptr → render only entities owned by that view
inline void render(entt::registry &registry, View *viewFilter = nullptr) {
    const auto &camera = registry.ctx<Camera>();
    auto &canvas = registry.ctx<M5Canvas>();

    canvas.clear();

    auto allSprites = registry.view<Position, Sprite>();
    std::for_each(allSprites.rbegin(), allSprites.rend(), [&](entt::entity e) {
        if (ownedBy(registry, e, viewFilter))
            drawEntity(registry, camera, canvas, e, allSprites.get<Position>(e), allSprites.get<Sprite>(e));
    });

    registry.view<Position, Label>().each([&](entt::entity e, const Position &pos, const Label &label) {
        if (!ownedBy(registry, e, viewFilter))
            return;
        canvas.setTextSize(label.size);
        canvas.setTextDatum(MC_DATUM);
        canvas.setTextColor(label.color);
        canvas.drawString(label.text, pos.x, pos.y);
    });
}

inline void present(entt::registry &registry) {
    const auto &camera = registry.ctx<Camera>();
    auto &canvas = registry.ctx<M5Canvas>();

    const uint16_t scaledW = min((uint32_t)camera.w * camera.scale, (uint32_t)M5.Display.width());
    const uint16_t scaledH = min((uint32_t)camera.h * camera.scale, (uint32_t)M5.Display.height());

    const int32_t dstX = (M5.Display.width() - scaledW) / 2;
    const int32_t dstY = (M5.Display.height() - scaledH) / 2;
    const uint16_t *src = static_cast<const uint16_t *>(canvas.getBuffer());

    static uint16_t rowBuf[320]; // one scaled row, bounded by display width

    M5.Display.startWrite();
    M5.Display.setWindow(dstX, dstY, dstX + scaledW - 1, dstY + scaledH - 1);

    for (uint16_t sy = 0; sy * camera.scale < scaledH; sy++) {
        const uint16_t *srcRow = src + sy * camera.w;

        // Expand each pixel horizontally by `scale`, stopping at scaledW
        uint16_t *p = rowBuf;
        uint16_t written = 0;
        for (uint16_t sx = 0; sx < camera.w && written < scaledW; sx++) {
            uint16_t px = srcRow[sx];
            for (uint8_t s = 0; s < camera.scale && written < scaledW; s++, written++)
                *p++ = px;
        }

        // Repeat the same expanded row `scale` times for vertical scaling
        for (uint8_t s = 0; s < camera.scale; s++)
            M5.Display.writePixels(rowBuf, scaledW);
    }

    M5.Display.endWrite();
}

// --- Debug ---

inline void renderHitboxes(entt::registry &registry) {
    const auto &camera = registry.ctx<Camera>();
    auto &canvas = registry.ctx<M5Canvas>();

    registry.view<Position, Sprite>().each([&](entt::entity e, const Position &pos, const Sprite &sprite) {
        int16_t sx = pos.x - camera.x;
        int16_t sy = pos.y - camera.y;
        // canvas.drawRect(sx, sy, sprite.w, sprite.h, TFT_CYAN);

        if (const Hitbox *hb = registry.try_get<Hitbox>(e))
            canvas.drawRect(sx + hb->ox, sy + hb->oy, hb->w, hb->h, TFT_WHITE);
    });
}

inline void showDebugOverlay(entt::registry &registry) {
    static uint32_t lastCheck = 0;
    static uint16_t fps = 0;
    static uint16_t frameCount = 0;
    static int8_t batteryLevel = -1;
    static bool charging = false;

    uint32_t now = millis();
    frameCount++;

    if (now - lastCheck >= 1000) {
        fps = frameCount * 1000 / (now - lastCheck);
        frameCount = 0;
        batteryLevel = M5.Power.getBatteryLevel();
        charging = M5.Power.isCharging();
        lastCheck = now;
    }

    auto &canvas = registry.ctx<M5Canvas>();
    const auto &camera = registry.ctx<Camera>();

    canvas.setTextSize(1);
    canvas.setTextDatum(TL_DATUM);
    canvas.setTextColor(TFT_WHITE, TFT_BLACK);

    // FPS — top left
    canvas.setCursor(2, 2);
    canvas.printf("FPS: %u", fps);

    // battery icon — top right
    if (batteryLevel >= 0) {
        int16_t x = camera.w - 18;
        int16_t y = 2;
        uint16_t color = charging ? TFT_GREEN : (batteryLevel < 20 ? TFT_RED : TFT_WHITE);
        canvas.drawRect(x, y, 14, 7, TFT_WHITE);
        canvas.fillRect(x + 14, y + 2, 2, 3, TFT_WHITE);
        canvas.fillRect(x + 1, y + 1, batteryLevel * 12 / 100, 5, color);
    }
}
