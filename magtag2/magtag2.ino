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
#define WIDTH 296
#define HEIGHT 128

#define COLOR1 EPD_BLACK
#define COLOR2 EPD_LIGHT
#define COLOR3 EPD_DARK

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

  switch(random(2)) {
    case 0: 
      displaySentence();
      break;
    case 1:
      displayMaze();
  }

  // Deep sleep for 600 seconds
  Serial.println("deep sleep");
  delay(1000);
  display.powerDown();
  digitalWrite(EPD_RESET, LOW); // hardware power down mode
  digitalWrite(SPEAKER_SHUTDOWN, LOW); // off
  digitalWrite(NEOPIXEL_POWER, HIGH); // off
  esp_sleep_enable_timer_wakeup(600 * 1000000);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_15, LOW);  // First button
  esp_deep_sleep_start();
}
void loop() {}

void displaySentence() {
  const char *verbs[] = {"Hug", "Kiss", "Throw", "Squish", "Pinch", "Read", "Talk to", "Clean up", "Find"};
  const char *arts[] = {"a"};
  const char *adjs[] = {"funny", "silly", "red", "soft", "sad", "hungry", "green", "noisy", "quiet", "light-up", "fuzzy", "yucky", "hard", "new"};
  const char *nouns[] = {"book", "toy", "car", "puzzle", "friend", "blanket", "pillow", "shoe", "sock", "hat", "sunglasses", "pencil", "crayon", "sticker"};
  const char *verb = pick(verbs, 9, "verbs");
  const char *art = pick(arts, 1, "arts");
  const char *adj = pick(adjs, 14, "adjs");
  const char *noun = pick(nouns, 14, "nouns");
  displayCentered(verb, art, adj, noun);
}

const char* pick(const char *arr[], size_t max, const char *arr_name) {
  long i = random(max);
  Serial.print(arr_name);
  Serial.print("[");
  Serial.print(i);
  Serial.print("] = ");
  Serial.println(arr[i]);
  return arr[i];
}

// Display is 296x128.
void displayCentered(const char* verb, const char* art, const char* adj, const char* noun) {
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

void displayMaze() {
  int mazesize = 10;
  int mw = WIDTH/mazesize;
  int mh = HEIGHT/mazesize;
  bool visited[mw*mh];
  bool downWall[mw*mh];
  bool rightWall[mw*mh];
  // Init
  for (int i=0; i<mw*mh; i++) {
    visited[i] = false;
    downWall[i] = true;
    rightWall[i] = true;
  }

  // TODO: maze algorithm.
  visited[0] = true;
  int numv = 1;
  // while there are any unvisited cells
  while (numv < mw * mh) {
  //while (numv < 10) {
    // pick a random unvisited cell
    int r = random(mw*mh - numv);
    int cell;
    for (cell=0; cell < mw*mh; cell++) {
      if (!visited[cell]) r--;
      if (r < 0) break;
    }
    Serial.print("picked cell ");
    Serial.println(cell);
    // if it has any visited neighbors, pick one at random (fisher-yates)
    int numvn = 0;
    int ncell = -1;
    int cellx = cell % mw;
    if (cellx > 0 && visited[cell-1] && random(++numvn) == 0)         ncell = cell-1;
    if (cellx < mw-1 && visited[cell+1] && random(++numvn) == 0)      ncell = cell+1;
    if (cell >= mw && visited[cell-mw] && random(++numvn) == 0)       ncell = cell-mw;
    if (cell < (mh-1)*mw && visited[cell+mw] && random(++numvn) == 0) ncell = cell+mw;
    if (ncell == -1) continue;
    Serial.print("visited neighbor is ");
    Serial.println(ncell);
    //   remove the wall between them
    if (ncell == cell-1) rightWall[cell-1] = false;
    if (ncell == cell+1) rightWall[cell] = false;
    if (ncell == cell-mw) downWall[cell-mw] = false;
    if (ncell == cell+mw) downWall[cell] = false;
    //   mark it as visited
    visited[cell] = true;
    numv++;
  }

  // Draw all the walls that weren't removed
  for (int x=0; x<mw; x++) {
    for (int y=0; y<mh; y++) {
      if (downWall[x+(y*mw)]) {
        // draw the bottom wall
        display.writeFastHLine(x*mazesize, (y+1)*mazesize, mazesize, EPD_BLACK);        
      }
      if (rightWall[x+(y*mw)]) {
        // draw the right wall
        display.writeFastVLine((x+1)*mazesize, y*mazesize, mazesize, EPD_BLACK);  
      }
    }
  }
  
  display.setCursor(2, 1);
  display.print("S");
  display.setCursor((mw-1)*mazesize+3, (mh-1)*mazesize+2);
  display.print("E");
  display.display();
}
