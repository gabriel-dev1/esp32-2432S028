#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

#define SPK_PIN 22

// TFT
TFT_eSPI tft = TFT_eSPI();

// TOUCH
#define XPT2046_CS   33
#define XPT2046_IRQ  36

SPIClass touchscreenSPI(VSPI);
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);

// DISPLAY
#define SCREEN_W 320
#define SCREEN_H 240

#define TOUCH_MIN_X  300
#define TOUCH_MAX_X  3800
#define TOUCH_MIN_Y  300
#define TOUCH_MAX_Y  3800

// AUDIO

void soundClick() {
  tone(SPK_PIN, 1200, 35);
}

void soundScroll() {
  tone(SPK_PIN, 900, 15);
}

void soundBack() {
  tone(SPK_PIN, 700, 60);
}

void soundError() {
  tone(SPK_PIN, 300, 180);
}


struct Content {
  const char* title;
  const char* body;
};

Content contents[] = {
  {
    "Automatos Finitos",
    "Um Automato Finito e um modelo matematico composto por estados e "
    "transicoes.\n\nCada entrada altera o estado atual.\n\n"
    "Eles reconhecem linguagens regulares."
  },
  {
    "Maquina de Turing",
    "Modelo teorico mais poderoso da computacao.\n\n"
    "Possui fita infinita e regras simples.\n\n"
    "Pode simular qualquer algoritmo."
  },
  {
    "Problema da Parada",
    "Nao existe algoritmo geral que determine se um programa vai parar.\n\n"
    "Esse limite foi provado por Alan Turing."
  },
  {
    "P vs NP",
    "Alguns problemas sao faceis de verificar, mas dificeis de resolver.\n\n"
    "Ainda nao sabemos se P = NP."
  },
  {
    "Filosofia da Computacao",
    "Computar e manipular simbolos.\n\n"
    "Mas significado nao esta na maquina."
  },
};

const int CONTENT_COUNT = sizeof(contents) / sizeof(contents[0]);

enum AppState {
  STATE_MENU,
  STATE_CONTENT
};

AppState state = STATE_MENU;
int selectedIndex = -1;

int scrollOffset = 0;
int lastTouchY = -1;
bool dragging = false;

const int ITEM_H = 48;
int touchStartY = -1;
bool moved = false;

const int SCROLL_THRESHOLD = 12; // px (crítico!)


void drawMenu() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawCentreString("Mini Plataforma Teorica", SCREEN_W / 2, 5, 2);

  for (int i = 0; i < CONTENT_COUNT; i++) {
    int y = 30 + i * ITEM_H - scrollOffset;

    if (y > -ITEM_H && y < SCREEN_H) {
      tft.fillRect(10, y, SCREEN_W - 20, ITEM_H - 6, TFT_BLUE);
      tft.drawRect(10, y, SCREEN_W - 20, ITEM_H - 6, TFT_WHITE);
      tft.drawCentreString(contents[i].title, SCREEN_W / 2, y + 14, 2);
    }
  }
}

void drawBackButton() {
  tft.fillRect(0, 0, 60, 30, TFT_DARKGREY);
  tft.drawRect(0, 0, 60, 30, TFT_WHITE);
  tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
  tft.drawCentreString("< Voltar", 30, 8, 2);
}


void drawContent() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  drawBackButton();

  tft.drawCentreString(contents[selectedIndex].title, SCREEN_W / 2, 5, 2);
  tft.drawFastHLine(0, 30, SCREEN_W, TFT_WHITE);

  tft.setCursor(10, 40 - scrollOffset);
  tft.setTextFont(2);
  tft.setTextWrap(true);

  tft.print(contents[selectedIndex].body);
}

bool readTouch(int &x, int &y) {
  if (!ts.touched()) return false;

  TS_Point p = ts.getPoint();
  x = map(p.x, TOUCH_MIN_X, TOUCH_MAX_X, 0, SCREEN_W);
  y = map(p.y, TOUCH_MIN_Y, TOUCH_MAX_Y, 0, SCREEN_H);
  return true;
}

void handleMenuTouch() {
  int x, y;

  // SOLTO
  if (!readTouch(x, y)) {

    // DECIDE CLIQUE APENAS AO SOLTAR
    if (!moved && touchStartY != -1) {
      int index = (touchStartY + scrollOffset - 30) / ITEM_H;

      if (index >= 0 && index < CONTENT_COUNT) {
        soundClick();
        selectedIndex = index;
        scrollOffset = 0;
        state = STATE_CONTENT;
        drawContent();
        delay(250);
      }
    }

    // RESET
    touchStartY = -1;
    lastTouchY = -1;
    moved = false;
    return;
  }

  // PRIMEIRO TOQUE
  if (touchStartY == -1) {
    touchStartY = y;
    lastTouchY = y;
    moved = false;
    return;
  }

  // MOVIMENTO 
  int delta = lastTouchY - y;

  if (abs(y - touchStartY) > SCROLL_THRESHOLD) {
    moved = true;
    static unsigned long lastScrollSound = 0;
  if (millis() - lastScrollSound > 120) {
    soundScroll();
    lastScrollSound = millis();
  }

  }

  if (moved) {
    scrollOffset += delta;
    scrollOffset = constrain(
      scrollOffset,
      0,
      max(0, CONTENT_COUNT * ITEM_H - (SCREEN_H - 40))
    );

    drawMenu();
  }

  lastTouchY = y;
}


void handleContentTouch() {
  int x, y;
  if (!readTouch(x, y)) {
    lastTouchY = -1;
    return;
  }

  // Botão voltar
  if (x < 60 && y < 30) {
    soundBack();
    state = STATE_MENU;
    scrollOffset = 0;
    drawMenu();
    delay(300);
    return;
  }

  // Scroll do conteúdo
  if (lastTouchY != -1) {
    soundScroll();
    int delta = lastTouchY - y;
    scrollOffset += delta;
    scrollOffset = constrain(scrollOffset, 0, 500);
    drawContent();
  }

  lastTouchY = y;
}


void setup() {
  Serial.begin(115200);

  tft.init();
  tft.setRotation(1);

  touchscreenSPI.begin(25, 39, 32, XPT2046_CS);
  ts.begin(touchscreenSPI);
  ts.setRotation(1);

  drawMenu();
}

void loop() {
  if (state == STATE_MENU) {
    handleMenuTouch();
  } else if (state == STATE_CONTENT) {
    handleContentTouch();
  }
}
