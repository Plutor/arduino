#include "Adafruit_ThinkInk.h"
#include <Adafruit_GFX.h>  // Core graphics library
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <stdio.h>

#define EPD_DC 7
#define EPD_CS 8
#define EPD_BUSY -1
#define SRAM_CS -1
#define EPD_RESET 6

// 2.9" Grayscale Featherwing or Breakout:
ThinkInk_290_Grayscale4_T5 display(EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_BUSY);
#define WIDTH 296
#define HEIGHT 128

// Deep sleep runs setup() again.
void setup() {
  // Random seed.
  randomSeed(analogRead(0));

  pinMode(BUTTON_A, INPUT_PULLUP);
  display.begin(THINKINK_GRAYSCALE4);
  display.clearBuffer();
  Serial.begin(115200);
  delay(2000);
  Serial.println("awake");

  switch (random(3)) {
    case 0:
      displayMath();
      break;
    case 1:
      displayMaze();
      break;
    case 2:
      displaySpelling();
  }

  // Deep sleep for 600 seconds
  Serial.println("deep sleep");
  delay(1000);
  display.powerDown();
  digitalWrite(EPD_RESET, LOW);         // hardware power down mode
  digitalWrite(SPEAKER_SHUTDOWN, LOW);  // off
  digitalWrite(NEOPIXEL_POWER, HIGH);   // off
  esp_sleep_enable_timer_wakeup(600 * 1000000L);
  // The first button, GPIO_NUM_15, is triggering a wake immediately on my
  // MagTag, whether I set to to LOW or HIGH...
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_14, LOW);  // Second button
  esp_deep_sleep_start();
}
void loop() {}

// ============================================================================
// MAZE

void displayMaze() {
  int mazesize = 10;
  int mw = WIDTH / mazesize;
  int mh = HEIGHT / mazesize;
  bool visited[mw * mh];
  bool downWall[mw * mh];
  bool rightWall[mw * mh];
  // Init
  for (int i = 0; i < mw * mh; i++) {
    visited[i] = false;
    downWall[i] = true;
    rightWall[i] = true;
  }

  visited[0] = true;
  int numv = 1;
  // while there are any unvisited cells
  while (numv < mw * mh) {
    // pick a random unvisited cell
    int r = random(mw * mh - numv);
    int cell;
    for (cell = 0; cell < mw * mh; cell++) {
      if (!visited[cell]) r--;
      if (r < 0) break;
    }
    Serial.print("picked cell ");
    Serial.println(cell);
    // if it has any visited neighbors, pick one at random (fisher-yates)
    int numvn = 0;
    int ncell = -1;
    int cellx = cell % mw;
    if (cellx > 0 && visited[cell - 1] && random(++numvn) == 0) ncell = cell - 1;
    if (cellx < mw - 1 && visited[cell + 1] && random(++numvn) == 0) ncell = cell + 1;
    if (cell >= mw && visited[cell - mw] && random(++numvn) == 0) ncell = cell - mw;
    if (cell < (mh - 1) * mw && visited[cell + mw] && random(++numvn) == 0) ncell = cell + mw;
    if (ncell == -1) continue;
    Serial.print("visited neighbor is ");
    Serial.println(ncell);
    //   remove the wall between them
    if (ncell == cell - 1) rightWall[cell - 1] = false;
    if (ncell == cell + 1) rightWall[cell] = false;
    if (ncell == cell - mw) downWall[cell - mw] = false;
    if (ncell == cell + mw) downWall[cell] = false;
    //   mark it as visited
    visited[cell] = true;
    numv++;
  }

  // Draw all the walls that weren't removed
  for (int x = 0; x < mw; x++) {
    for (int y = 0; y < mh; y++) {
      if (downWall[x + (y * mw)]) {
        // draw the bottom wall
        display.writeFastHLine(x * mazesize, (y + 1) * mazesize, mazesize, EPD_DARK);
      }
      if (rightWall[x + (y * mw)]) {
        // draw the right wall
        display.writeFastVLine((x + 1) * mazesize, y * mazesize, mazesize, EPD_DARK);
      }
    }
  }

  display.setTextColor(EPD_BLACK);
  display.setCursor(2, 1);
  display.print("S");
  display.setCursor((mw - 1) * mazesize + 3, (mh - 1) * mazesize + 2);
  display.print("E");
  display.display();
}

// ============================================================================
// MATH

void displayMath() {
  if (random(2) == 0) {
    displayMath1stGrade();
  } else {
    displayMath3rdGrade();
  }
}

// Only addition and subtraction, one number <20, the other <10
void displayMath1stGrade() {
  int num1 = random(19) + 1;
  int num2 = random(9) + 1;
  if (random(2) == 0) {
    // Addition
    displayMathString(num1, '+', num2);
  } else {
    // Subtraction
    if (num2 > num1) {
      int t = num1;
      num1 = num2;
      num2 = num1;
    }
    displayMathString(num1, '-', num2);
  }
}

// Addition and subtraction up to 999. Also multiplication up to 12*12 and
// division up to 99/9 (no remainders).
void displayMath3rdGrade() {
  int op = random(4);
  switch (op) {
    case 0:
      {
        // Addition
        displayMathString(random(999) + 1, '+', random(999) + 1);
        break;
      }
    case 1:
      {
        // Subtraction
        int num1 = random(999) + 1;
        displayMathString(num1, '-', random(num1) + 1);
        break;
      }
    case 2:
      {
        // Multiplication
        displayMathString(random(12) + 1, 'x', random(12) + 1);
        break;
      }
    case 3:
      {
        // Division
        int div = random(12) + 1;
        displayMathString(div * (random(12) + 1), '/', div);
        break;
      }
  }
}

void displayMathString(int num1, char op, int num2) {
  int16_t x, y;
  uint16_t w, h;
  char str[256];
  sprintf(str, "%d %c %d = ?", num1, op, num2);
  display.setFont(&FreeSans18pt7b);
  display.setTextColor(EPD_BLACK);
  display.getTextBounds(str, 0, 0, &x, &y, &w, &h);
  display.setCursor((WIDTH - w) / 2, (HEIGHT + h) / 2);
  display.print(str);
  // display.setFont(&FreeMono9pt7b);
  // display.setCursor(0, HEIGHT);
  // sprintf(str, "%d,%d,%d,%d = ?", x,y,w,h);
  // display.print(str);
  display.display();
}

// ============================================================================
// SPELLING

const char* starts[] = { "CO", "RE", "IN", "DE", "UN", "PR", "CA", "DI", "ST",
                         "MA", "PA", "SU", "CH", "TR", "BA", "MI", "PE", "PO",
                         "SE", "MO", "SP", "AN", "HA", "SH", "ME", "SC", "EX",
                         "HO", "CR", "SA" };
void displaySpelling() {
  const char* start = starts[random(30)];
  char str[16];
  sprintf(str, "%s____?", start);

  int16_t x, y;
  uint16_t w1, w2, h1, h2;
  display.setFont(&FreeSans12pt7b);
  display.getTextBounds("How many words start with", 0, 0, &x, &y, &w1, &h1);
  display.setFont(&FreeSans18pt7b);
  display.getTextBounds(str, 0, 0, &x, &y, &w2, &h2);

  display.setTextColor(EPD_DARK);
  display.setFont(&FreeSans12pt7b);
  display.setCursor((WIDTH - w1) / 2, (HEIGHT + h1 + h2) / 2 - h2 - 2);
  display.print("How many words start with");
  display.setTextColor(EPD_BLACK);
  display.setFont(&FreeSans18pt7b);
  display.setCursor((WIDTH - w2) / 2, (HEIGHT + h1 + h2) / 2 + 2);
  display.print(str);
  display.display();
}
