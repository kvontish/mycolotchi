#pragma once

struct ButtonEvent
{
    enum class Button : uint8_t { A, B, C };
    enum class Action : uint8_t { Pressed, Released };

    Button button;
    Action action;
};

// Volatile flags written by the input task (Core 0),
// read and cleared by pollInput (Core 1).
extern volatile bool gBtnPressed[3];
