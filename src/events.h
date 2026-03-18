#pragma once

// Volatile state written by the input task (Core 0), read and cleared by pollInput (Core 1).
enum class ButtonState : uint8_t { None, Pressed, LongPressed };
extern volatile ButtonState gButtonState[3];

struct ButtonEvent {
    enum class Button : uint8_t { A, B, C };

    Button button;
    ButtonState action; // never ButtonState::None
};
