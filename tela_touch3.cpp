#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <WiFi.h>
#include <esp_now.h>

/* ===================== CONFIG GERAL ===================== */

#define SCREEN_W 320
#define SCREEN_H 240
#define SPK_PIN 22

#define TOUCH_MIN_X 300
#define TOUCH_MAX_X 3800
#define TOUCH_MIN_Y 300
#define TOUCH_MAX_Y 3800

/* ===================== DISPLAY / TOUCH ===================== */

TFT_eSPI tft = TFT_eSPI();

#define XPT2046_CS 33
#define XPT2046_IRQ 36

SPIClass touchscreenSPI(VSPI);
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);

/* Functions declarations */
void game2Render();
void drawMenu();

/* ===================== AUDIO ===================== */

void soundClick()  { tone(SPK_PIN, 1200, 40); }
void soundScroll() { tone(SPK_PIN, 900, 20); }
void soundBack()   { tone(SPK_PIN, 700, 80); }

/* ===================== TOUCH ===================== */

bool readTouch(int &x, int &y) {
  if (!ts.touched()) return false;
  TS_Point p = ts.getPoint();
  x = map(p.x, TOUCH_MIN_X, TOUCH_MAX_X, 0, SCREEN_W);
  y = map(p.y, TOUCH_MIN_Y, TOUCH_MAX_Y, 0, SCREEN_H);
  return true;
}

/* ===================== ESP-NOW ===================== */

struct EnvPacket {
  float temperature;
  float humidity;
};

struct Environment {
  float temperature = 0;
  float humidity = 0;
  bool updated = false;
};

Environment env;

void onReceive(const uint8_t* mac, const uint8_t* data, int len) {
  if (len != sizeof(EnvPacket)) return;
  memcpy(&env, data, sizeof(env));
  env.updated = true;
}

void setupESPNow() {
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Erro ESP-NOW");
    return;
  }
  esp_now_register_recv_cb(onReceive);
}

/* ===================== SISTEMA DE JOGOS ===================== */

enum AppState {
  STATE_MENU,
  STATE_GAME
};

AppState state = STATE_MENU;

struct Game {
  const char* name;
  void (*init)();
  void (*update)();
  void (*render)();
  void (*onTouch)(int, int);
};

Game* currentGame = nullptr;

/* ===================== GAME 1 (BASE) ===================== */

int nodeX = 160;
int nodeY = 120;
int nodeR = 22;
int clicks = 0;

void game1Init() {
  clicks = 0;
}

void game1Update() {}

void game1Render() {
  tft.fillScreen(TFT_WHITE);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.drawCentreString("Puzzle Logico", 160, 5, 2);

  tft.fillCircle(nodeX, nodeY, nodeR, TFT_BLUE);
  tft.drawString("Cliques:", 10, 200, 2);
  tft.drawNumber(clicks, 90, 200, 2);

  tft.fillRect(0, 0, 60, 30, TFT_DARKGREY);
  tft.drawCentreString("< Voltar", 30, 8, 2);
}

void game1Touch(int x, int y) {
  if (x < 60 && y < 30) {
    soundBack();
    state = STATE_MENU;
    drawMenu();
    currentGame = nullptr;
    return;
  }

  int dx = x - nodeX;
  int dy = y - nodeY;
  if (dx*dx + dy*dy <= nodeR*nodeR) {
    clicks++;
    soundClick();
    game1Render();
  }
}

Game game1 = {
  "Puzzle",
  game1Init,
  game1Update,
  game1Render,
  game1Touch
};

/* ===================== GAME 2 – LOGICA INSTAVEL ===================== */

// Variáveis lógicas
int A = 0;
int B = 0;
int C = 0;
int noise = 0;
int F = 0;
bool structureValid = false;

// Botões touch (coordenadas fixas)
#define BTN_W 80
#define BTN_H 40

// Posições
#define BTN_A_X 10
#define BTN_B_X 110
#define BTN_C_X 210
#define BTN_Y   200

#define BTN_RST_X 110
#define BTN_RST_Y 150

void game2Init() {
  A = B = C = 0;
  noise = F = 0;
  structureValid = false;
}

void computeGame2Logic() {
  noise = ((int)(env.temperature * 10) + (int)env.humidity) % 8;
  F = (A * 2 + B * 3 + C) ^ noise;
  structureValid = (F % 4 == 0);
}

void game2Update() {
  if (env.updated) {
    env.updated = false;
    computeGame2Logic();
    game2Render();
  }
}

void drawButton(int x, int y, const char* label, uint16_t color) {
  tft.fillRect(x, y, BTN_W, BTN_H, color);
  tft.setTextColor(TFT_BLACK, color);
  tft.drawCentreString(label, x + BTN_W / 2, y + 12, 2);
}

void game2Render() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);

  tft.drawString("GAME 2: LOGICA INSTAVEL", 10, 5, 2);

  // Valores lógicos
  tft.drawString("A: " + String(A), 10, 40, 2);
  tft.drawString("B: " + String(B), 10, 60, 2);
  tft.drawString("C: " + String(C), 10, 80, 2);

  // Sensor
  tft.drawString("Temp: " + String(env.temperature, 1), 10, 110, 2);
  tft.drawString("Hum: " + String(env.humidity, 0), 10, 130, 2);

  tft.drawString("Noise: " + String(noise), 10, 155, 2);
  tft.drawString("F: " + String(F), 10, 175, 2);

  // Estado
  if (structureValid) {
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.drawString("VALID STRUCTURE", 150, 110, 2);
  } else {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.drawString("UNSTABLE STRUCTURE", 150, 110, 2);
  }

  // Botões
  drawButton(BTN_A_X, BTN_Y, "A +1", TFT_BLUE);
  drawButton(BTN_B_X, BTN_Y, "B +1", TFT_GREEN);
  drawButton(BTN_C_X, BTN_Y, "C +1", TFT_ORANGE);
  drawButton(BTN_RST_X, BTN_RST_Y, "RESET", TFT_RED);

  // Voltar
  tft.fillRect(0, 0, 60, 30, TFT_DARKGREY);
  tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
  tft.drawCentreString("< Voltar", 30, 8, 2);
}

bool touchIn(int x, int y, int rx, int ry, int rw, int rh) {
  return (x >= rx && x <= rx + rw && y >= ry && y <= ry + rh);
}

void game2Touch(int x, int y) {
  // Voltar
  if (x < 60 && y < 30) {
    soundBack();
    state = STATE_MENU;
    drawMenu();
    currentGame = nullptr;
    return;
  }

  if (touchIn(x, y, BTN_A_X, BTN_Y, BTN_W, BTN_H)) {
    A = (A + 1) % 8;
    soundClick();
  }
  else if (touchIn(x, y, BTN_B_X, BTN_Y, BTN_W, BTN_H)) {
    B = (B + 1) % 8;
    soundClick();
  }
  else if (touchIn(x, y, BTN_C_X, BTN_Y, BTN_W, BTN_H)) {
    C = (C + 1) % 8;
    soundClick();
  }
  else if (touchIn(x, y, BTN_RST_X, BTN_RST_Y, BTN_W, BTN_H)) {
    A = B = C = 0;
    soundBack();
  }

  computeGame2Logic();
  game2Render();
}

Game game2 = {
  "Logica Instavel",
  game2Init,
  game2Update,
  game2Render,
  game2Touch
};


/* ===================== MENU ===================== */

void drawMenu() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  tft.drawCentreString("Mini Plataforma", 160, 10, 2);

  tft.fillRect(40, 70, 240, 40, TFT_BLUE);
  tft.drawCentreString("Puzzle Logico", 160, 82, 2);

  tft.fillRect(40, 130, 240, 40, TFT_GREEN);
  tft.drawCentreString("Logica Instavel", 160, 142, 2);
}

void handleMenuTouch() {
  int x, y;
  if (!readTouch(x, y)) return;

  if (y > 70 && y < 110) {
    soundClick();
    currentGame = &game1;
  }
  else if (y > 130 && y < 170) {
    soundClick();
    currentGame = &game2;
  }

  if (currentGame) {
    state = STATE_GAME;
    currentGame->init();
    currentGame->render();
    delay(200);
  }
}

/* ===================== SETUP / LOOP ===================== */

void setup() {
  Serial.begin(115200);

  tft.init();
  tft.setRotation(1);

  touchscreenSPI.begin(25, 39, 32, XPT2046_CS);
  ts.begin(touchscreenSPI);
  ts.setRotation(1);

  setupESPNow();
  drawMenu();
}

void loop() {
  if (state == STATE_MENU) {
    handleMenuTouch();
  }
  else if (state == STATE_GAME && currentGame) {
    currentGame->update();
    int x, y;
    if (readTouch(x, y)) {
      currentGame->onTouch(x, y);
      delay(120);
    }
  }
}
