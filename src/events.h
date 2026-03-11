#pragma once

struct ButtonEvent
{
    enum class Button : uint8_t { A, B, C };
    enum class Action : uint8_t { Pressed, Released };

    Button button;
    Action action;
};
