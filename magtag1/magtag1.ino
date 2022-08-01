#include "Adafruit_ThinkInk.h"
#include <Adafruit_GFX.h>    // Core graphics library
#include <Fonts/FreeSans18pt7b.h>

#define EPD_DC      7 
#define EPD_CS      8
#define EPD_BUSY    -1
#define SRAM_CS     -1 
#define EPD_RESET   6

// 2.9" Grayscale Featherwing or Breakout:
ThinkInk_290_Grayscale4_T5 display(EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_BUSY);

#define COLOR1 EPD_BLACK
#define COLOR2 EPD_LIGHT
#define COLOR3 EPD_DARK

const char *verbs[] = {"Hug", "Kiss", "Throw", "Squish", "Pinch", "Read", "Talk to", "Clean up", "Find"};
const char *arts[] = {"a"};
const char *adjs[] = {"funny", "silly", "red", "soft", "sad", "hungry", "green", "noisy", "quiet", "light-up", "fuzzy", "yucky", "hard", "new"};
const char *nouns[] = {"book", "toy", "car", "puzzle", "friend", "blanket", "pillow", "shoe", "sock", "hat", "sunglasses", "pencil", "crayon", "sticker"};


// Deep sleep runs setup() again.
void setup() {
  // Random seed.
  randomSeed(analogRead(0));

  pinMode(BUTTON_A, INPUT_PULLUP);
  display.begin(THINKINK_GRAYSCALE4);
  Serial.begin(115200);
  // while (!Serial) { delay(10); }
  Serial.println("awake");

  const char *verb = pick(verbs, 9, "verbs");
  const char *art = pick(arts, 1, "arts");
  const char *adj = pick(adjs, 14, "adjs");
  const char *noun = pick(nouns, 14, "nouns");

  displayCentered(verb, art, adj, noun);
  delay(2000);

  // Deep sleep for 60 seconds
  Serial.println("deep sleep");
  display.powerDown();
  digitalWrite(EPD_RESET, LOW); // hardware power down mode
  digitalWrite(SPEAKER_SHUTDOWN, LOW); // off
  digitalWrite(NEOPIXEL_POWER, HIGH); // off
  esp_sleep_enable_timer_wakeup(600 * 1000000);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_15, LOW);  // First button
  esp_deep_sleep_start();
}

const char* pick(const char *arr[], size_t max, const char *arr_name) {
  long i= random(max);
  Serial.print(arr_name);
  Serial.print("[");
  Serial.print(i);
  Serial.print("] = ");
  Serial.println(arr[i]);
  return arr[i];
}

// Display is 296x128.
void displayCentered(const char* verb, const char* art, const char* adj, const char* noun) {
  display.clearBuffer();
  display.setFont(&FreeSans18pt7b);
  display.setTextColor(COLOR1);

  char *line1 = (char*)malloc(strlen(verb) + strlen(art) + 2);
  strcpy(line1, verb);
  strcat(line1, " ");
  strcat(line1, art);
  char *line2 = (char*)malloc(strlen(adj) + strlen(noun) + 3);
  strcpy(line2, adj);
  strcat(line2, " ");
  strcat(line2, noun);
  strcat(line2, ".");

  int16_t  x1, y1;
  uint16_t w, h;
  display.getTextBounds(line1, 0, 0, &x1, &y1, &w, &h);
  display.setCursor(148 - (w/2), 62);
  display.print(line1);
  display.getTextBounds(line2, 0, 0, &x1, &y1, &w, &h);
  display.setCursor(148 - (w/2), 65+h);
  display.print(line2);
  display.display();
}

void loop() {}
