#include <M5Unified.h>

void setup()
{
    M5.begin();
    M5.Display.println("Hello, World!");
    Serial.begin(115200);
    Serial.println("Hello, World!");
}

void loop()
{
    M5.update();
}
