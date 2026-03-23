#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

Adafruit_NeoPixel led(1, 48, NEO_GRB + NEO_KHZ800);
// uint8_t brightness = 10;

void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.printf("Flash size:  %d MB\n", ESP.getFlashChipSize() / (1024 * 1024));
  Serial.printf("PSRAM size:  %d MB\n", ESP.getPsramSize() / (1024 * 1024));
  Serial.printf("Free heap:   %d bytes\n", ESP.getFreeHeap());
  Serial.printf("Free PSRAM:  %d bytes\n", ESP.getFreePsram());

  led.begin();
  led.clear();
  led.setBrightness(0); // 0-255
  led.show();
}

void loop()
{
  // led.setPixelColor(0, led.Color(255, 0, 0)); // Rojo
  // led.show();
  // delay(1000);
  // led.setBrightness(brightness); // 0-255
  // brightness += 10;

  // led.setPixelColor(0, led.Color(0, 255, 0)); // Verde
  // led.show();
  // delay(1000);
  // led.setBrightness(brightness); // 0-255
  // brightness += 10;

  // led.setPixelColor(0, led.Color(0, 0, 255)); // Azul
  // led.show();
  // delay(1000);
  // led.setBrightness(brightness); // 0-255
  // brightness += 10;
}
