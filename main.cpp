#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

// ================= TFT =================
TFT_eSPI tft = TFT_eSPI();

// ================= TOUCH =================
#define XPT2046_CS   33
#define XPT2046_IRQ  36

SPIClass touchscreenSPI(VSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);

// ================= SCREEN =================
#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 240
#define FONT_SIZE 2

int posX, posY, pressure;
int centerX, centerY;

// ================= CALIBRAÇÃO REAL =================
// AJUSTE SE NECESSÁRIO
#define TOUCH_MIN_X  300
#define TOUCH_MAX_X  3800
#define TOUCH_MIN_Y  300
#define TOUCH_MAX_Y  3800

// ================= LOG =================
void logTouchData(int x, int y, int z)
{
  Serial.printf("X=%d | Y=%d | Z=%d\n", x, y, z);
}

// ================= DISPLAY =================
void displayTouchData(int x, int y, int z)
{
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  tft.drawCentreString("Touch Test", centerX, 10, FONT_SIZE);

  tft.drawCentreString(
    "X=" + String(x) + " Y=" + String(y),
    centerX, 40, FONT_SIZE
  );

  tft.drawCentreString(
    "Pressure=" + String(z),
    centerX, 60, FONT_SIZE
  );

  // Área válida
  tft.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, TFT_BLUE);

  // Ponto
  tft.fillCircle(x, y, 5, TFT_RED);
}

// ================= SETUP =================
void setup()
{
  Serial.begin(115200);
  delay(1000);

  // TFT
  tft.init();
  tft.setRotation(1);

  // Touch SPI
  touchscreenSPI.begin(25, 39, 32, XPT2046_CS);
  touchscreen.begin(touchscreenSPI);
  touchscreen.setRotation(1);

  centerX = SCREEN_WIDTH / 2;
  centerY = SCREEN_HEIGHT / 2;

  tft.fillScreen(TFT_BLACK);
  tft.drawCentreString("ILI9341 + XPT2046", centerX, centerY - 10, FONT_SIZE);
  tft.drawCentreString("Touch the screen", centerX, centerY + 10, FONT_SIZE);
}

// ================= LOOP =================
void loop()
{
  if (touchscreen.touched())
  {
    TS_Point p = touchscreen.getPoint();

    posX = map(p.x, TOUCH_MIN_X, TOUCH_MAX_X, 0, SCREEN_WIDTH);
    posY = map(p.y, TOUCH_MIN_Y, TOUCH_MAX_Y, 0, SCREEN_HEIGHT);
    pressure = p.z;

    // Limites
    posX = constrain(posX, 0, SCREEN_WIDTH);
    posY = constrain(posY, 0, SCREEN_HEIGHT);

    logTouchData(posX, posY, pressure);
    displayTouchData(posX, posY, pressure);

    delay(50);
  }
}

