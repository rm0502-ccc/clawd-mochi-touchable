/*
 * ╔══════════════════════════════════════════════════════════════╗
 * CLAWD MOCHI — ESP32-C3 Super Mini + ST7789 1.54" 240×240
 *
 * Wiring:
 * SDA → GPIO 10  (hardware SPI MOSI)
 * SCL → GPIO 8   (hardware SPI SCK)
 * RST → GPIO 2
 * DC  → GPIO 1
 * CS  → GPIO 4
 * BL  → GPIO 3
 * VCC → 3V3
 * GND → GND
 *
 * WiFi: "ClaWD-Mochi"  pw: clawd1234  → http://192.168.4.1
 * ╚══════════════════════════════════════════════════════════════╝
 */

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <math.h>
#include <WiFi.h>
#include <WebServer.h>
#include <time.h>

// ── Pins ──────────────────────────────────────────────────────
#define TFT_CS  4
#define TFT_DC  1
#define TFT_RST 2
#define TFT_BLK 3

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

#define TOUCH_PIN 5   // TTP223 SIG → GPIO 5

// ── WiFi ──────────────────────────────────────────────────────
const char* WIFI_SSID = "your_wifi_name";   // ← 改成你家WiFi名称
const char* WIFI_PASS = "your_wifi_password";   // ← 改成你家WiFi密码
WebServer server(80);

// ── Display ───────────────────────────────────────────────────
#define DISP_W 240
#define DISP_H 240

// ── Eye constants (shared by both eye views) ──────────────────
#define EYE_W   30
#define EYE_H   60
#define EYE_GAP 120
#define EYE_OX  0     // horizontal offset
#define EYE_OY  40    // vertical offset upward (subtracted from centre)

// ── Colours ───────────────────────────────────────────────────
uint16_t C_ORANGE, C_DARKBG, C_MUTED, C_GREEN;

#define C_WHITE ST77XX_WHITE
#define C_BLACK ST77XX_BLACK

// ── State ─────────────────────────────────────────────────────
#define VIEW_EYES_NORMAL  0
#define VIEW_EYES_SQUISH  1
#define VIEW_CODE         2
#define VIEW_DRAW         3
#define VIEW_EYES_ANGRY   4
#define VIEW_EYES_SAD     5
#define VIEW_EYES_TIRED   6
#define VIEW_EYES_SLEEP   7
#define VIEW_EYES_THINK   8
#define VIEW_EYES_HAPPY   9
#define VIEW_EYES_ANNOYED 10
#define VIEW_EYES_KISS    11
#define VIEW_EYES_WINK    12
#define VIEW_EYES_BUBBLE  13
#define VIEW_EYES_BORED    14
#define VIEW_EYES_CONFUSED 15
#define VIEW_EYES_DIZZY    16
#define VIEW_EYES_DEAD     17
#define VIEW_EYES_LOOKAROUND 18

uint8_t  currentView  = VIEW_EYES_NORMAL;
bool     busy         = false;
bool     backlightOn  = true;
uint8_t  animSpeed    = 1;   // 1=slow(default) 2=normal 3=fast

// ── Auto-cycle ────────────────────────────────────────────────
unsigned long lastExprMs    = 0;
unsigned long nextExprMs    = 10000;  // first switch after 10s
bool          manualOverride = false;
unsigned long overrideEnd   = 0;
int8_t        lastTimeHour  = -1;     // track hour changes
uint8_t       exprPlayCount = 0;      // how many times current expr has played
#define       EXPR_REPEAT   10        // play same expr this many times before switching

// ── Touch ─────────────────────────────────────────────────────
bool          lastTouchState = false;
unsigned long lastTouchMs    = 0;
#define TOUCH_DEBOUNCE_MS 50
// Touch overload tracking (5+ touches within 25s → dizzy, reset 20s after last touch)
uint8_t       touchCount     = 0;
unsigned long firstTouchMs   = 0;
bool          touchOverload  = false;
bool          touchInterrupt = false;  // true = touch detected, animation should exit
#define TOUCH_WINDOW_MS  25000
#define TOUCH_OVERLOAD   5
#define TOUCH_RESET_MS   20000

uint16_t animBgColor  = 0;   // background for eye/logo animations
uint16_t drawBgColor  = 0;   // background for canvas

// ── Terminal ──────────────────────────────────────────────────
#define TERM_COLS      15
#define TERM_ROWS       8
#define TERM_CHAR_W    12
#define TERM_CHAR_H    20
#define TERM_PAD_X      8
#define TERM_PAD_Y     18

bool    termMode    = false;
String  termLines[TERM_ROWS];
uint8_t termRow     = 0;
uint8_t termCol     = 0;

// ── Logo data ─────────────────────────────────────────────────
#define LOGO_CX 120
#define LOGO_CY 105

#define LOGO_TRI_COUNT 162
static const int16_t LOGO_TRIS[][6] PROGMEM = {
  {120,105,65,134,100,114},{120,105,100,114,101,113},{120,105,101,113,100,112},
  {120,105,100,112,99,112},{120,105,99,112,93,111},{120,105,93,111,73,111},
  {120,105,73,111,55,110},{120,105,55,110,38,109},{120,105,38,109,34,108},
  {120,105,34,108,30,103},{120,105,30,103,30,100},{120,105,30,100,34,98},
  {120,105,34,98,39,98},{120,105,39,98,50,99},{120,105,50,99,67,100},
  {120,105,67,100,80,101},{120,105,80,101,98,103},{120,105,98,103,101,103},
  {120,105,101,103,101,102},{120,105,101,102,100,101},{120,105,100,101,100,100},
  {120,105,100,100,82,88},{120,105,82,88,63,76},{120,105,63,76,53,69},
  {120,105,53,69,48,65},{120,105,48,65,45,61},{120,105,45,61,44,54},
  {120,105,44,54,49,49},{120,105,49,49,55,49},{120,105,55,49,57,49},
  {120,105,57,49,64,55},{120,105,64,55,78,66},{120,105,78,66,96,79},
  {120,105,96,79,99,81},{120,105,99,81,100,81},{120,105,100,81,100,80},
  {120,105,100,80,99,78},{120,105,99,78,89,60},{120,105,89,60,78,41},
  {120,105,78,41,73,34},{120,105,73,34,72,29},{120,105,72,29,72,28},
  {120,105,72,28,72,27},{120,105,72,27,71,26},{120,105,71,26,71,25},
  {120,105,71,25,71,24},{120,105,71,24,77,16},{120,105,77,16,80,15},
  {120,105,80,15,87,16},{120,105,87,16,91,19},{120,105,91,19,95,29},
  {120,105,95,29,103,46},{120,105,103,46,114,68},{120,105,114,68,118,75},
  {120,105,118,75,119,81},{120,105,119,81,120,83},{120,105,120,83,121,83},
  {120,105,121,83,121,82},{120,105,121,82,122,69},{120,105,122,69,124,54},
  {120,105,124,54,126,34},{120,105,126,34,126,28},{120,105,126,28,129,21},
  {120,105,129,21,135,18},{120,105,135,18,139,20},{120,105,139,20,143,25},
  {120,105,143,25,142,28},{120,105,142,28,140,42},{120,105,140,42,136,64},
  {120,105,136,64,133,78},{120,105,133,78,135,78},{120,105,135,78,136,76},
  {120,105,136,76,144,67},{120,105,144,67,156,51},{120,105,156,51,162,45},
  {120,105,162,45,168,38},{120,105,168,38,172,35},{120,105,172,35,180,35},
  {120,105,180,35,185,43},{120,105,185,43,183,52},{120,105,183,52,175,62},
  {120,105,175,62,168,71},{120,105,168,71,159,83},{120,105,159,83,153,94},
  {120,105,153,94,154,94},{120,105,154,94,155,94},{120,105,155,94,176,90},
  {120,105,176,90,188,88},{120,105,188,88,201,85},{120,105,201,85,208,88},
  {120,105,208,88,208,91},{120,105,208,91,206,97},{120,105,206,97,191,101},
  {120,105,191,101,174,104},{120,105,174,104,148,110},{120,105,148,110,148,111},
  {120,105,148,111,148,111},{120,105,148,111,160,112},{120,105,160,112,165,112},
  {120,105,165,112,177,112},{120,105,177,112,200,114},{120,105,200,114,205,118},
  {120,105,205,118,209,123},{120,105,209,123,208,126},{120,105,208,126,199,131},
  {120,105,199,131,187,128},{120,105,187,128,159,121},{120,105,159,121,149,119},
  {120,105,149,119,147,119},{120,105,147,119,147,120},{120,105,147,120,156,128},
  {120,105,156,128,170,141},{120,105,170,141,189,158},{120,105,189,158,190,163},
  {120,105,190,163,188,166},{120,105,188,166,185,166},{120,105,185,166,169,153},
  {120,105,169,153,162,148},{120,105,162,148,148,136},{120,105,148,136,147,136},
  {120,105,147,136,147,137},{120,105,147,137,150,142},{120,105,150,142,168,168},
  {120,105,168,168,169,176},{120,105,169,176,168,179},{120,105,168,179,163,180},
  {120,105,163,180,158,179},{120,105,158,179,148,165},{120,105,148,165,137,149},
  {120,105,137,149,129,134},{120,105,129,134,128,135},{120,105,128,135,123,189},
  {120,105,123,189,120,192},{120,105,120,192,115,194},{120,105,115,194,110,191},
  {120,105,110,191,108,185},{120,105,108,185,110,174},{120,105,110,174,113,160},
  {120,105,113,160,116,148},{120,105,116,148,118,134},{120,105,118,134,119,129},
  {120,105,119,129,119,129},{120,105,119,129,118,129},{120,105,118,129,107,144},
  {120,105,107,144,91,166},{120,105,91,166,78,180},{120,105,78,180,75,181},
  {120,105,75,181,70,178},{120,105,70,178,70,173},{120,105,70,173,73,169},
  {120,105,73,169,91,146},{120,105,91,146,102,132},{120,105,102,132,109,124},
  {120,105,109,124,109,123},{120,105,109,123,108,123},{120,105,108,123,61,153},
  {120,105,61,153,52,155},{120,105,52,155,49,151},{120,105,49,151,49,146},
  {120,105,49,146,51,144},{120,105,51,144,65,134},{120,105,65,134,65,134},
};
#define LOGO_SEG_COUNT 162
static const int16_t LOGO_SEGS[][4] PROGMEM = {
  {65,134,100,114},{100,114,101,113},{101,113,100,112},{100,112,99,112},
  {99,112,93,111},{93,111,73,111},{73,111,55,110},{55,110,38,109},
  {38,109,34,108},{34,108,30,103},{30,103,30,100},{30,100,34,98},
  {34,98,39,98},{39,98,50,99},{50,99,67,100},{67,100,80,101},
  {80,101,98,103},{98,103,101,103},{101,103,101,102},{101,102,100,101},
  {100,101,100,100},{100,100,82,88},{82,88,63,76},{63,76,53,69},
  {53,69,48,65},{48,65,45,61},{45,61,44,54},{44,54,49,49},
  {49,49,55,49},{55,49,57,49},{57,49,64,55},{64,55,78,66},
  {78,66,96,79},{96,79,99,81},{99,81,100,81},{100,81,100,80},
  {100,80,99,78},{99,78,89,60},{89,60,78,41},{78,41,73,34},
  {73,34,72,29},{72,29,72,28},{72,28,72,27},{72,27,71,26},
  {71,26,71,25},{71,25,71,24},{71,24,77,16},{77,16,80,15},
  {80,15,87,16},{87,16,91,19},{91,19,95,29},{95,29,103,46},
  {103,46,114,68},{114,68,118,75},{118,75,119,81},{119,81,120,83},
  {120,83,121,83},{121,83,121,82},{121,82,122,69},{122,69,124,54},
  {124,54,126,34},{126,34,126,28},{126,28,129,21},{129,21,135,18},
  {135,18,139,20},{139,20,143,25},{143,25,142,28},{142,28,140,42},
  {140,42,136,64},{136,64,133,78},{133,78,135,78},{135,78,136,76},
  {136,76,144,67},{144,67,156,51},{156,51,162,45},{162,45,168,38},
  {168,38,172,35},{172,35,180,35},{180,35,185,43},{185,43,183,52},
  {183,52,175,62},{175,62,168,71},{168,71,159,83},{159,83,153,94},
  {153,94,154,94},{154,94,155,94},{155,94,176,90},{176,90,188,88},
  {188,88,201,85},{201,85,208,88},{208,88,208,91},{208,91,206,97},
  {206,97,191,101},{191,101,174,104},{174,104,148,110},{148,110,148,111},
  {148,111,148,111},{148,111,160,112},{160,112,165,112},{165,112,177,112},
  {177,112,200,114},{200,114,205,118},{205,118,209,123},{209,123,208,126},
  {208,126,199,131},{199,131,187,128},{187,128,159,121},{159,121,149,119},
  {149,119,147,119},{147,119,147,120},{147,120,156,128},{156,128,170,141},
  {170,141,189,158},{189,158,190,163},{190,163,188,166},{188,166,185,166},
  {185,166,169,153},{169,153,162,148},{162,148,148,136},{148,136,147,136},
  {147,136,147,137},{147,137,150,142},{150,142,168,168},{168,168,169,176},
  {169,176,168,179},{168,179,163,180},{163,180,158,179},{158,179,148,165},
  {148,165,137,149},{137,149,129,134},{129,134,128,135},{128,135,123,189},
  {123,189,120,192},{120,192,115,194},{115,194,110,191},{110,191,108,185},
  {108,185,110,174},{110,174,113,160},{113,160,116,148},{116,148,118,134},
  {118,134,119,129},{119,129,119,129},{119,129,118,129},{118,129,107,144},
  {107,144,91,166},{91,166,78,180},{78,180,75,181},{75,181,70,178},
  {70,178,70,173},{70,173,73,169},{73,169,91,146},{91,146,102,132},
  {102,132,109,124},{109,124,109,123},{109,123,108,123},{108,123,61,153},
  {61,153,52,155},{52,155,49,151},{49,151,49,146},{49,146,51,144},
  {51,144,65,134},{65,134,65,134},
};

// ═════════════════════════════════════════════════════════════
//  HELPERS
// ═════════════════════════════════════════════════════════════

int speedMs(int ms) {
  if (animSpeed == 3) return ms / 2;
  if (animSpeed == 1) return ms * 2;
  return ms;
}

uint16_t hexToRgb565(String hex) {
  hex.replace("#", "");
  if (hex.length() != 6) return C_WHITE;
  long v = strtol(hex.c_str(), nullptr, 16);
  return tft.color565((v >> 16) & 0xFF, (v >> 8) & 0xFF, v & 0xFF);
}

void setBacklight(bool on) {
  backlightOn = on;
  digitalWrite(TFT_BLK, on ? HIGH : LOW);
}

void initColours() {
  C_ORANGE = tft.color565(218, 17, 0);
  C_DARKBG = tft.color565(10,  12,  16);
  C_MUTED  = tft.color565(90,  88,  86);
  C_GREEN  = tft.color565(80, 220, 130);
  animBgColor = C_ORANGE;
  drawBgColor = C_ORANGE;
}

// ═════════════════════════════════════════════════════════════
//  LOGO
// ═════════════════════════════════════════════════════════════

void drawLogoFilled(uint16_t bg, uint16_t fg) {
  tft.fillScreen(bg);
  for (uint16_t i = 0; i < LOGO_TRI_COUNT; i++) {
    tft.fillTriangle(
      pgm_read_word(&LOGO_TRIS[i][0]), pgm_read_word(&LOGO_TRIS[i][1]),
      pgm_read_word(&LOGO_TRIS[i][2]), pgm_read_word(&LOGO_TRIS[i][3]),
      pgm_read_word(&LOGO_TRIS[i][4]), pgm_read_word(&LOGO_TRIS[i][5]),
      fg);
  }
  tft.setTextColor(fg); tft.setTextSize(2);
  tft.setCursor(LOGO_CX - 54, 210); tft.print("Anthropic");
  tft.setCursor(LOGO_CX - 53, 210); tft.print("Anthropic");
}

// ═════════════════════════════════════════════════════════════
//  VIEWS
// ═════════════════════════════════════════════════════════════

// Eye helpers — shared constants via #define EYE_*
inline int16_t eyeLX(int16_t ox) {
  return (DISP_W - (EYE_W * 2 + EYE_GAP)) / 2 + EYE_OX + ox;
}
inline int16_t eyeRX(int16_t ox) { return eyeLX(ox) + EYE_W + EYE_GAP; }
inline int16_t eyeY()            { return (DISP_H - EYE_H) / 2 - EYE_OY; }
inline int16_t eyeCY()           { return eyeY() + EYE_H / 2; }

void drawNormalEyes(int16_t ox = 0, bool blink = false) {
  tft.fillScreen(animBgColor);
  const int16_t lx = eyeLX(ox), rx = eyeRX(ox), ey = eyeY();
  if (!blink) {
    tft.fillRect(lx, ey, EYE_W, EYE_H, C_BLACK);
    tft.fillRect(rx, ey, EYE_W, EYE_H, C_BLACK);
  } else {
    tft.fillRect(lx, ey + EYE_H / 2 - 3, EYE_W, 6, C_BLACK);
    tft.fillRect(rx, ey + EYE_H / 2 - 3, EYE_W, 6, C_BLACK);
  }
}

void drawChevron(int16_t cx, int16_t cy, int16_t arm, int16_t reach,
                 uint8_t thk, bool rightFacing, uint16_t col) {
  for (int8_t t = -(int8_t)thk; t <= (int8_t)thk; t++) {
    if (rightFacing) {
      tft.drawLine(cx - reach/2, cy - arm + t, cx + reach/2, cy + t,      col);
      tft.drawLine(cx + reach/2, cy + t,       cx - reach/2, cy + arm + t, col);
    } else {
      tft.drawLine(cx + reach/2, cy - arm + t, cx - reach/2, cy + t,      col);
      tft.drawLine(cx - reach/2, cy + t,       cx + reach/2, cy + arm + t, col);
    }
  }
}

void drawSquishEyes(int16_t ox = 0) {
  tft.fillScreen(animBgColor);
  const int16_t lx = eyeLX(ox), rx = eyeRX(ox), cy = eyeCY();
  const int16_t arm = EYE_H / 3, reach = EYE_W;
  drawChevron(lx + EYE_W/2, cy, arm, reach, 10, true,  C_BLACK);
  drawChevron(rx + EYE_W/2, cy, arm, reach, 10, false, C_BLACK);
}

void drawCodeView() {
  termMode = false;
  tft.fillScreen(C_DARKBG);
  tft.fillRect(0, 0,          DISP_W, 4, C_ORANGE);
  tft.fillRect(0, DISP_H - 4, DISP_W, 4, C_ORANGE);
  tft.setTextColor(C_ORANGE); tft.setTextSize(4);
  tft.setCursor((DISP_W - 144) / 2, DISP_H / 2 - 52); tft.print("Claude");
  tft.setTextColor(C_WHITE);  tft.setTextSize(4);
  tft.setCursor((DISP_W - 96) / 2,  DISP_H / 2 + 8);  tft.print("Code");
  tft.fillRect((DISP_W - 96) / 2, DISP_H / 2 + 52, 96, 3, C_ORANGE);
}

// ═════════════════════════════════════════════════════════════
//  TERMINAL
// ═════════════════════════════════════════════════════════════

void termClear() {
  for (uint8_t i = 0; i < TERM_ROWS; i++) termLines[i] = "";
  termRow = 0; termCol = 0;
}

void termDrawHeader() {
  tft.fillRect(0, 0, DISP_W, TERM_PAD_Y + 1, C_DARKBG);
  tft.setTextColor(C_ORANGE); tft.setTextSize(1);
  tft.setCursor(TERM_PAD_X, 4); tft.print("clawd@mochi terminal");
  tft.drawFastHLine(0, TERM_PAD_Y, DISP_W, C_ORANGE);
}

void termDrawPrefix(int16_t yy) {
  tft.setTextColor(C_GREEN);
  tft.setTextSize(1);
  tft.setCursor(TERM_PAD_X, yy + 6);
  tft.print("clawd:~$ ");
}

#define PREFIX_PX 54

void termDrawLine(uint8_t r) {
  const int16_t yy = TERM_PAD_Y + 4 + r * TERM_CHAR_H;
  tft.fillRect(0, yy, DISP_W, TERM_CHAR_H, C_DARKBG);
  if (r == termRow) termDrawPrefix(yy);
  tft.setTextColor(C_WHITE); tft.setTextSize(2);
  tft.setCursor(TERM_PAD_X + PREFIX_PX, yy + 1);
  tft.print(termLines[r]);
  if (r == termRow) {
    const int16_t cx = TERM_PAD_X + PREFIX_PX + termCol * TERM_CHAR_W;
    tft.fillRect(cx, yy + 1, TERM_CHAR_W - 2, TERM_CHAR_H - 2, C_GREEN);
  }
}

void termDrawLastChar() {
  if (termCol == 0) return;
  const int16_t yy    = TERM_PAD_Y + 4 + termRow * TERM_CHAR_H;
  const int16_t baseX = TERM_PAD_X + PREFIX_PX;
  const uint8_t prev  = termCol - 1;
  tft.fillRect(baseX + prev * TERM_CHAR_W, yy + 1, TERM_CHAR_W, TERM_CHAR_H - 1, C_DARKBG);
  tft.setTextColor(C_WHITE); tft.setTextSize(2);
  tft.setCursor(baseX + prev * TERM_CHAR_W, yy + 1);
  tft.print(termLines[termRow][prev]);
  tft.fillRect(baseX + termCol * TERM_CHAR_W, yy + 1, TERM_CHAR_W - 2, TERM_CHAR_H - 2, C_GREEN);
}

void termDrawBackspace() {
  const int16_t yy    = TERM_PAD_Y + 4 + termRow * TERM_CHAR_H;
  const int16_t baseX = TERM_PAD_X + PREFIX_PX;
  tft.fillRect(baseX + termCol * TERM_CHAR_W, yy + 1, TERM_CHAR_W * 2, TERM_CHAR_H - 1, C_DARKBG);
  tft.fillRect(baseX + termCol * TERM_CHAR_W, yy + 1, TERM_CHAR_W - 2, TERM_CHAR_H - 2, C_GREEN);
  if (termLines[termRow].length() == 0) {
    tft.fillRect(0, yy, TERM_PAD_X + PREFIX_PX, TERM_CHAR_H, C_DARKBG);
  }
}

void termFullRedraw() {
  tft.fillScreen(C_DARKBG);
  termDrawHeader();
  for (uint8_t r = 0; r < TERM_ROWS; r++) termDrawLine(r);
}

void termScroll() {
  for (uint8_t i = 0; i < TERM_ROWS - 1; i++) termLines[i] = termLines[i + 1];
  termLines[TERM_ROWS - 1] = "";
  termRow = TERM_ROWS - 1;
  termFullRedraw();
}

void termAddChar(char c) {
  if (c == '\n' || c == '\r') {
    const int16_t yy = TERM_PAD_Y + 4 + termRow * TERM_CHAR_H;
    tft.fillRect(TERM_PAD_X + PREFIX_PX + termCol * TERM_CHAR_W,
                 yy + 1, TERM_CHAR_W, TERM_CHAR_H - 1, C_DARKBG);
    termRow++; termCol = 0;
    if (termRow >= TERM_ROWS) { termScroll(); return; }
    termDrawLine(termRow);
  } else if (c == '\b' || c == 127) {
    if (termCol > 0) {
      termCol--;
      termLines[termRow].remove(termLines[termRow].length() - 1);
      termDrawBackspace();
    }
  } else if (c >= 32 && c < 127) {
    if (termCol >= TERM_COLS) {
      termRow++;
      termCol = 0;
      if (termRow >= TERM_ROWS) { termScroll(); return; }
    }
    if (termCol == 0) termDrawPrefix(TERM_PAD_Y + 4 + termRow * TERM_CHAR_H);
    termLines[termRow] += c;
    termCol++;
    termDrawLastChar();
  }
}

// ═════════════════════════════════════════════════════════════
//  ANIMATIONS
// ═════════════════════════════════════════════════════════════

// Check touch sensor during animation delays; sets touchInterrupt if touched
void checkTouchSensor() {
  bool touchNow = (bool)digitalRead(TOUCH_PIN);
  if (touchNow && !lastTouchState && (millis() - lastTouchMs > TOUCH_DEBOUNCE_MS)) {
    touchInterrupt = true;
  }
  lastTouchState = touchNow;
}

// Delay that can be interrupted by touch
bool animDelay(int ms) {
  ms = speedMs(ms);
  unsigned long start = millis();
  while (millis() - start < (unsigned long)ms) {
    checkTouchSensor();
    if (touchInterrupt) return true;  // interrupted
    delay(1);
  }
  return false;
}

void animNormalEyes() {
  busy = true;
  const int16_t offs[] = {-16, 16, -16, 16, 0};
  for (uint8_t i = 0; i < 5; i++) { drawNormalEyes(offs[i]); delay(speedMs(80)); }
  drawNormalEyes(0, true);  delay(speedMs(100));
  drawNormalEyes(0, false); delay(speedMs(70));
  drawNormalEyes(0, true);  delay(speedMs(70));
  drawNormalEyes(0, false);
  busy = false;
}

void animSquishEyes() {
  busy = true;
  const int16_t offs[] = {0, -20, 20, -16, 16, -10, 10, -5, 5, 0};
  for (uint8_t i = 0; i < 10; i++) { drawSquishEyes(offs[i]); delay(speedMs(90)); }
  drawSquishEyes(0);
  busy = false;
}

void animLookaroundEyes() {
  busy = true;
  // Left-right look around
  const int16_t offs[] = {0, -16, -16, 16, 16, -16, 16, 0};
  for (uint8_t i = 0; i < 8; i++) { drawNormalEyes(offs[i]); delay(speedMs(120)); }
  drawNormalEyes(0);
  busy = false;
}

void drawAngryEyes(int16_t ox = 0) {
  tft.fillScreen(animBgColor);
  const int16_t lx = eyeLX(ox), rx = eyeRX(ox), ey = eyeY();
  tft.fillRect(lx, ey + 12, EYE_W, EYE_H - 12, C_BLACK);
  tft.fillRect(rx, ey + 12, EYE_W, EYE_H - 12, C_BLACK);
  for (int8_t t = -4; t <= 4; t++) {
    tft.drawLine(lx - 6, ey + t,      lx + EYE_W + 2, ey + 14 + t, C_BLACK);
    tft.drawLine(rx - 2, ey + 14 + t, rx + EYE_W + 6, ey + t,      C_BLACK);
  }
}
void animAngryEyes() {
  busy = true;
  const int16_t shk[] = {-10, 10, -8, 8, -5, 5, -3, 3, 0};
  for (uint8_t i = 0; i < 9; i++) { drawAngryEyes(shk[i]); delay(speedMs(55)); }
  drawAngryEyes(0);
  busy = false;
}

void drawSadEyes(uint8_t tearY = 0) {
  tft.fillScreen(animBgColor);
  const int16_t lx = eyeLX(0), rx = eyeRX(0), ey = eyeY();
  tft.fillRect(lx, ey + EYE_H / 2, EYE_W, EYE_H / 2, C_BLACK);
  tft.fillRect(rx, ey + EYE_H / 2, EYE_W, EYE_H / 2, C_BLACK);
  for (int8_t t = -3; t <= 3; t++) {
    tft.drawLine(lx,          ey + 10 + t, lx + EYE_W, ey + t,      C_BLACK);
    tft.drawLine(rx,          ey + t,      rx + EYE_W, ey + 10 + t, C_BLACK);
  }
  if (tearY > 0) {
    uint16_t tc = tft.color565(80, 160, 255);
    tft.fillRect(lx + EYE_W / 2 - 2, ey + EYE_H + tearY, 4, 10, tc);
    tft.fillRect(rx + EYE_W / 2 - 2, ey + EYE_H + tearY, 4, 10, tc);
  }
}
void animSadEyes() {
  busy = true;
  drawSadEyes(0); delay(speedMs(400));
  for (uint8_t t = 0; t <= 24; t += 4) { drawSadEyes(t); delay(speedMs(80)); }
  drawSadEyes(24); delay(speedMs(500));
  drawSadEyes(0);
  busy = false;
}

void drawTiredEyes(uint8_t droop = 30) {
  tft.fillScreen(animBgColor);
  const int16_t lx = eyeLX(0), rx = eyeRX(0), ey = eyeY();
  uint8_t h = (droop >= EYE_H - 4) ? 4 : (EYE_H - droop);
  tft.fillRect(lx, ey + EYE_H - h, EYE_W, h, C_BLACK);
  tft.fillRect(rx, ey + EYE_H - h, EYE_W, h, C_BLACK);
}
void animTiredEyes() {
  busy = true;
  for (uint8_t d = 0; d <= 44; d += 4) { drawTiredEyes(d); if (animDelay(70)) { busy = false; return; } }
  if (animDelay(600)) { busy = false; return; }
  for (uint8_t d = 44; d > 30; d -= 4) { drawTiredEyes(d); if (animDelay(70)) { busy = false; return; } }
  drawTiredEyes(30);
  busy = false;
}

void drawSleepEyes(uint8_t zzzCount = 0) {
  tft.fillScreen(animBgColor);
  const int16_t lx = eyeLX(0), rx = eyeRX(0), cy = eyeCY();
  // Thinner lines, same position as tired last frame
  const uint8_t slpH = EYE_H / 3;
  tft.fillRect(lx, cy, EYE_W, slpH, C_BLACK);
  tft.fillRect(rx, cy, EYE_W, slpH, C_BLACK);
  // z's float upward from eye level
  tft.setTextColor(C_BLACK);
  if (zzzCount >= 1) { tft.setTextSize(1); tft.setCursor(rx + EYE_W + 4,  cy - 6);  tft.print("z"); }
  if (zzzCount >= 2) { tft.setTextSize(2); tft.setCursor(rx + EYE_W + 10, cy - 22); tft.print("z"); }
  if (zzzCount >= 3) { tft.setTextSize(3); tft.setCursor(rx + EYE_W + 18, cy - 42); tft.print("Z"); }
}
void animSleepEyes() {
  busy = true;
  drawSleepEyes(0); if (animDelay(400))  { busy = false; return; }
  drawSleepEyes(1); if (animDelay(600))  { busy = false; return; }
  drawSleepEyes(2); if (animDelay(600))  { busy = false; return; }
  drawSleepEyes(3); if (animDelay(900))  { busy = false; return; }
  drawSleepEyes(1); if (animDelay(300))  { busy = false; return; }
  drawSleepEyes(0);
  busy = false;
}

void drawThinkEyes(int16_t ox = 0, uint8_t bubble = 0) {
  tft.fillScreen(animBgColor);
  const int16_t lx = eyeLX(ox), rx = eyeRX(ox), ey = eyeY();
  const int16_t cy = eyeCY();
  // Both eyes look upward (rect shifted up, shorter)
  tft.fillRect(lx, ey, EYE_W, EYE_H - 10, C_BLACK);
  tft.fillRect(rx, ey, EYE_W, EYE_H - 10, C_BLACK);
  // Thought cloud: dots getting bigger as they go up-right
  if (bubble >= 1) tft.fillCircle(rx + EYE_W + 6,  ey + 20,  3, C_BLACK);
  if (bubble >= 2) tft.fillCircle(rx + EYE_W + 14, ey + 4,   5, C_BLACK);
  if (bubble >= 3) {
    // Big thought bubble (single circle)
    int16_t cx = rx + EYE_W + 22, ccy = ey - 14;
    tft.fillCircle(cx, ccy, 10, C_BLACK);
  }
}
void animThinkEyes() {
  busy = true;
  drawThinkEyes(0, 0); delay(speedMs(400));
  // Bubbles appear one by one
  drawThinkEyes(0, 1); delay(speedMs(250));
  drawThinkEyes(0, 2); delay(speedMs(250));
  drawThinkEyes(0, 3); delay(speedMs(600));
  // Eyes drift while thinking
  for (int8_t o = 0; o <= 16; o += 4) { drawThinkEyes(o, 3); delay(speedMs(80)); }
  drawThinkEyes(16, 3); delay(speedMs(400));
  for (int8_t o = 16; o >= -16; o -= 4) { drawThinkEyes(o, 3); delay(speedMs(80)); }
  drawThinkEyes(-16, 3); delay(speedMs(400));
  // Look back center, bubbles fade
  for (int8_t o = -16; o <= 0; o += 4) { drawThinkEyes(o, 3); delay(speedMs(80)); }
  drawThinkEyes(0, 3); delay(speedMs(200));
  drawThinkEyes(0, 2); delay(speedMs(150));
  drawThinkEyes(0, 1); delay(speedMs(150));
  drawThinkEyes(0, 0);
  busy = false;
}

// 垂直于线段方向的等宽描边
void solidSeg(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t thk) {
  float dx = x2-x1, dy = y2-y1;
  float len = sqrtf(dx*dx + dy*dy);
  if (len < 1.0f) return;
  int16_t px = (int16_t)roundf(-dy/len * thk);
  int16_t py = (int16_t)roundf( dx/len * thk);
  tft.fillTriangle(x1+px, y1+py, x1-px, y1-py, x2+px, y2+py, C_BLACK);
  tft.fillTriangle(x1-px, y1-py, x2-px, y2-py, x2+px, y2+py, C_BLACK);
}

void drawHappyEyes(int16_t oy = 0) {
  tft.fillScreen(animBgColor);
  const int16_t lcx = eyeLX(0) + EYE_W/2;
  const int16_t rcx = eyeRX(0) + EYE_W/2;
  const int16_t cy  = eyeCY() + oy;
  const int16_t hw  = EYE_W/2 + 2;
  const int16_t arm = EYE_H/3;
  solidSeg(lcx-hw, cy+arm, lcx,    cy-arm, 5);
  solidSeg(lcx,    cy-arm, lcx+hw, cy+arm, 5);
  solidSeg(rcx-hw, cy+arm, rcx,    cy-arm, 5);
  solidSeg(rcx,    cy-arm, rcx+hw, cy+arm, 5);
}

void animHappyEyes() {
  busy = true;
  const int16_t b[] = {0, -10, 0, -8, 0, -6, 0, -4, 0};
  for (uint8_t i = 0; i < 9; i++) { drawHappyEyes(b[i]); delay(speedMs(75)); }
  drawHappyEyes(0);
  busy = false;
}

void drawAnnoyedEyes(int16_t ox = 0) {
  tft.fillScreen(animBgColor);
  const int16_t lx = eyeLX(ox), rx = eyeRX(ox), ey = eyeY();
  tft.fillRect(lx, ey, EYE_W, EYE_H, C_BLACK);
  tft.fillRect(rx, ey + EYE_H / 2, EYE_W, EYE_H / 2, C_BLACK);
  for (int8_t t = -3; t <= 3; t++)
    tft.drawLine(rx - 2, ey + 10 + t, rx + EYE_W + 2, ey + t, C_BLACK);
}
void animAnnoyedEyes() {
  busy = true;
  drawAnnoyedEyes(0); delay(speedMs(300));
  for (int8_t ox = 0; ox <= 12; ox += 3) { drawAnnoyedEyes(ox); delay(speedMs(80)); }
  for (int8_t ox = 12; ox >= -12; ox -= 3) { drawAnnoyedEyes(ox); delay(speedMs(80)); }
  for (int8_t ox = -12; ox <= 0; ox += 3) { drawAnnoyedEyes(ox); delay(speedMs(80)); }
  drawAnnoyedEyes(0);
  busy = false;
}

// Draw a ♥ heart - sharper and less round
void drawHeart(int16_t cx, int16_t cy, uint8_t sz, uint16_t col) {
  // Two smaller circles with wider gap for deep indent
  int16_t r  = sz * 4 / 7;           // smaller bump radius (less round)
  int16_t bx = sz * 3 / 7;           // wider horizontal offset for deeper indent
  tft.fillCircle(cx - bx, cy - r/2, r, col);
  tft.fillCircle(cx + bx, cy - r/2, r, col);
  // Sharp triangle pointing down
  tft.fillTriangle(cx - sz, cy, cx + sz, cy, cx, cy + sz + r, col);
}

void drawKissEyes(uint8_t phase = 0, uint8_t hs = 0) {
  tft.fillScreen(animBgColor);
  const int16_t lx = eyeLX(0), rx = eyeRX(0), ey = eyeY();
  const int16_t cy = eyeCY();
  // Left eye: normal rect |
  tft.fillRect(lx, ey, EYE_W, EYE_H, C_BLACK);
  // Right eye: < chevron (same as squish)
  drawChevron(rx + EYE_W/2, cy, EYE_H / 3, EYE_W, 10, false, C_BLACK);
  // Heart below eyes
  if (hs > 0) {
    uint16_t pk = tft.color565(255, 80, 140);
    int16_t hx = DISP_W / 2, hy = cy + EYE_H / 2 + 20;
    if (phase <= 1) {
      // Normal heart (phase 0) or pulsing bigger (phase 1)
      uint8_t sz = hs + phase * 2;
      drawHeart(hx, hy, sz, pk);
    } else if (phase == 2) {
      // Burst: expanding ring + scattered particles
      drawHeart(hx, hy, hs, pk);
      // White flash ring
      tft.drawCircle(hx, hy, hs + 4, C_WHITE);
      tft.drawCircle(hx, hy, hs + 6, C_WHITE);
    } else if (phase >= 3) {
      // Exploding particles flying outward
      uint16_t gl = tft.color565(255, 180, 200);
      uint8_t spread = (phase - 2) * 8;
      // Scattered heart fragments (small circles)
      tft.fillCircle(hx - spread, hy - spread/2, 3, pk);
      tft.fillCircle(hx + spread, hy - spread/2, 3, pk);
      tft.fillCircle(hx - spread/2, hy - spread, 2, pk);
      tft.fillCircle(hx + spread/2, hy - spread, 2, pk);
      tft.fillCircle(hx - spread, hy + spread/3, 2, gl);
      tft.fillCircle(hx + spread, hy + spread/3, 2, gl);
      tft.fillCircle(hx, hy + spread, 2, gl);
      // Sparkle dots
      tft.fillCircle(hx - spread - 4, hy - 6, 1, C_WHITE);
      tft.fillCircle(hx + spread + 4, hy - 4, 1, C_WHITE);
      tft.fillCircle(hx - 2, hy - spread - 3, 1, C_WHITE);
    }
  }
}
void animKissEyes() {
  busy = true;
  // Eyes appear
  drawKissEyes(0, 0); delay(speedMs(300));
  // Heart grows
  for (uint8_t s = 3; s <= 12; s += 2) { drawKissEyes(0, s); delay(speedMs(70)); }
  drawKissEyes(0, 12); delay(speedMs(600));
  // Heart pulses bigger
  drawKissEyes(1, 12); delay(speedMs(200));
  // Burst flash
  drawKissEyes(2, 12); delay(speedMs(100));
  // Particles explode outward
  drawKissEyes(3, 12); delay(speedMs(80));
  drawKissEyes(4, 12); delay(speedMs(80));
  drawKissEyes(5, 12); delay(speedMs(120));
  // Clear
  drawKissEyes(0, 0); delay(speedMs(400));
  busy = false;
}

void drawWinkEyes(bool blink = false) {
  tft.fillScreen(animBgColor);
  const int16_t lx = eyeLX(0), rx = eyeRX(0), ey = eyeY();
  const int16_t cy = eyeCY();
  if (!blink) {
    tft.fillRect(lx, ey, EYE_W, EYE_H, C_BLACK);
    tft.fillRect(rx, ey, EYE_W, EYE_H, C_BLACK);
  } else {
    tft.fillRect(lx, cy - 3, EYE_W, 6, C_BLACK);
    tft.fillRect(rx, cy - 3, EYE_W, 6, C_BLACK);
  }
}
void animWinkEyes() {
  busy = true;
  drawWinkEyes(false); delay(speedMs(600));
  drawWinkEyes(true);  delay(speedMs(120));
  drawWinkEyes(false); delay(speedMs(500));
  drawWinkEyes(true);  delay(speedMs(120));
  drawWinkEyes(false); delay(speedMs(400));
  drawWinkEyes(true);  delay(speedMs(120));
  drawWinkEyes(false);
  busy = false;
}

void drawBubbleEyes(uint8_t phase = 0) {
  tft.fillScreen(animBgColor);
  const int16_t lx = eyeLX(0), rx = eyeRX(0), ey = eyeY();
  tft.fillRect(lx, ey, EYE_W, EYE_H, C_BLACK);
  tft.fillRect(rx, ey, EYE_W, EYE_H, C_BLACK);
  uint16_t bc = tft.color565(120, 190, 255);
  int16_t base = ey + EYE_H + 8;
  int16_t bxs[] = { (int16_t)(lx + EYE_W/2 - 8), (int16_t)(lx + EYE_W/2 + 8),
                    (int16_t)(rx + EYE_W/2 - 8), (int16_t)(rx + EYE_W/2 + 8) };
  uint8_t rs[] = {5, 4, 6, 4};
  for (uint8_t i = 0; i < 4; i++) {
    int16_t by = base - ((phase + i * 18) % 90);
    if (by > ey - 12 && by < base + 10)
      tft.drawCircle(bxs[i], by, rs[i], bc);
  }
}
void animBubbleEyes() {
  busy = true;
  for (uint8_t p = 0; p < 72; p += 3) { drawBubbleEyes(p); delay(speedMs(55)); }
  drawBubbleEyes(0);
  busy = false;
}

void drawBoredEyes(int16_t ox = 0, int16_t prevOx = 0) {
  tft.fillScreen(animBgColor);
  const int16_t lx = eyeLX(ox), rx = eyeRX(ox), ey = eyeY();
  uint8_t h = EYE_H * 2 / 5;
  tft.fillRect(lx, ey + EYE_H - h, EYE_W, h, C_BLACK);
  tft.fillRect(rx, ey + EYE_H - h, EYE_W, h, C_BLACK);
  // 根据移动方向决定倾斜：向左移动→\形，向右移动→/形
  int16_t delta = ox - prevOx;
  int16_t ly = (delta < 0) ? -4 : 0;
  int16_t ry = (delta < 0) ?  0 : -4;
  for (int8_t t = -2; t <= 2; t++) {
    tft.drawLine(lx, ey+EYE_H-h+ly+t, lx+EYE_W, ey+EYE_H-h+ry+t, C_BLACK);
    tft.drawLine(rx, ey+EYE_H-h+ly+t, rx+EYE_W, ey+EYE_H-h+ry+t, C_BLACK);
  }
}
void animBoredEyes() {
  busy = true;
  int16_t prev = 0;
  drawBoredEyes(0, 0); delay(speedMs(500));
  for (int8_t ox = 0; ox <= 24; ox += 4) { prev = ox; drawBoredEyes(ox, ox-4); delay(speedMs(100)); }
  drawBoredEyes(24, 24); delay(speedMs(800));
  prev = 24;
  for (int8_t ox = 24; ox >= -24; ox -= 4) { int16_t p = prev; prev = ox; drawBoredEyes(ox, p); delay(speedMs(100)); }
  drawBoredEyes(-24, -24); delay(speedMs(800));
  prev = -24;
  for (int8_t ox = -24; ox <= 0; ox += 4) { int16_t p = prev; prev = ox; drawBoredEyes(ox, p); delay(speedMs(100)); }
  drawBoredEyes(0, 0);
  busy = false;
}

// ── 疑问 Confused ─────────────────────────────────────────────
void drawConfusedEyes(int16_t ox = 0, bool showQ = false) {
  tft.fillScreen(animBgColor);
  const int16_t lx = eyeLX(ox), rx = eyeRX(ox), ey = eyeY();
  tft.fillRect(lx, ey, EYE_W, EYE_H, C_BLACK);
  tft.fillRect(rx, ey, EYE_W, EYE_H, C_BLACK);
  if (showQ) {
    tft.setTextColor(C_BLACK); tft.setTextSize(3);
    tft.setCursor(eyeRX(ox) + EYE_W + 2, ey - 4);
    tft.print("?");
  }
}
void animConfusedEyes() {
  busy = true;
  drawConfusedEyes(0, true); delay(speedMs(400));
  for (int8_t o = 0; o <= 20; o += 4) { drawConfusedEyes(o, o >= 8); delay(speedMs(70)); }
  drawConfusedEyes(20, true); delay(speedMs(500));
  for (int8_t o = 20; o >= -20; o -= 4) { drawConfusedEyes(o, o <= 0); delay(speedMs(70)); }
  drawConfusedEyes(-20, true); delay(speedMs(400));
  for (int8_t o = -20; o <= 0; o += 4) { drawConfusedEyes(o, false); delay(speedMs(70)); }
  drawConfusedEyes(0, false);
  busy = false;
}

// ── 晕 Dizzy ──────────────────────────────────────────────────
void drawDizzyEyes(uint8_t phase = 0) {
  tft.fillScreen(animBgColor);
  const int16_t lcx = eyeLX(0) + EYE_W / 2;
  const int16_t rcx = eyeRX(0) + EYE_W / 2;
  const int16_t cy  = eyeCY();
  const uint8_t maxR = EYE_W / 2 + 4;
  // Draw thick spiral for each eye using parametric spiral
  // Spiral: r = a + b*theta, offset by phase for animation
  float angleOffset = phase * 0.8f;
  for (float theta = 0; theta < 6.28f * 2.5f; theta += 0.15f) {
    float r = 2.0f + theta * 1.8f;
    if (r > maxR) break;
    float a = theta + angleOffset;
    int16_t px = (int16_t)(cosf(a) * r);
    int16_t py = (int16_t)(sinf(a) * r);
    // Draw 3px thick dots along spiral
    tft.fillCircle(lcx + px, cy + py, 2, C_BLACK);
    tft.fillCircle(rcx + px, cy + py, 2, C_BLACK);
  }
}
void animDizzyEyes() {
  busy = true;
  for (uint8_t p = 0; p < 24; p++) {
    drawDizzyEyes(p % 8);
    delay(speedMs(90));
  }
  drawDizzyEyes(0);
  busy = false;
}

// ── 死掉 Dead ─────────────────────────────────────────────────
void drawDeadEyes(int16_t ox = 0) {
  tft.fillScreen(animBgColor);
  const int16_t cy  = eyeCY();
  const int16_t arm = EYE_W / 2;        // 15px — 刚好在眼睛范围内
  const int16_t lcx = eyeLX(ox) + EYE_W / 2;
  const int16_t rcx = eyeRX(ox) + EYE_W / 2;
  const int8_t  thk = 5;
  for (int8_t t = -thk; t <= thk; t++) {
    // \ 对角线，垂直偏移方向 (+t, -t)
    tft.drawLine(lcx-arm+t, cy-arm-t, lcx+arm+t, cy+arm-t, C_BLACK);
    tft.drawLine(rcx-arm+t, cy-arm-t, rcx+arm+t, cy+arm-t, C_BLACK);
    // / 对角线，垂直偏移方向 (+t, +t)
    tft.drawLine(lcx+arm+t, cy-arm+t, lcx-arm+t, cy+arm+t, C_BLACK);
    tft.drawLine(rcx+arm+t, cy-arm+t, rcx-arm+t, cy+arm+t, C_BLACK);
  }
}
void animDeadEyes() {
  busy = true;
  tft.fillScreen(C_WHITE); delay(speedMs(70));
  tft.fillScreen(animBgColor); delay(speedMs(70));
  tft.fillScreen(C_WHITE); delay(speedMs(70));
  drawDeadEyes(0); delay(speedMs(400));
  const int16_t shk[] = {-8, 8, -6, 6, -4, 4, -2, 2, 0};
  for (uint8_t i = 0; i < 9; i++) { drawDeadEyes(shk[i]); delay(speedMs(50)); }
  drawDeadEyes(0);
  busy = false;
}

int8_t getHour() {
  struct tm t;
  if (!getLocalTime(&t)) return -1;
  return t.tm_hour;
}

void runExpr(uint8_t v) {
  currentView = v;
  switch (v) {
    case VIEW_EYES_NORMAL:   animNormalEyes();   break;
    case VIEW_EYES_SQUISH:   animSquishEyes();   break;
    case VIEW_EYES_ANGRY:    animAngryEyes();    break;
    case VIEW_EYES_SAD:      animSadEyes();      break;
    case VIEW_EYES_TIRED:    animTiredEyes();    break;
    case VIEW_EYES_SLEEP:    animSleepEyes();    break;
    case VIEW_EYES_THINK:    animThinkEyes();    break;
    case VIEW_EYES_HAPPY:    animHappyEyes();    break;
    case VIEW_EYES_ANNOYED:  animAnnoyedEyes();  break;
    case VIEW_EYES_KISS:     animKissEyes();     break;
    case VIEW_EYES_WINK:     animWinkEyes();     break;
    case VIEW_EYES_BORED:    animBoredEyes();    break;
    case VIEW_EYES_CONFUSED: animConfusedEyes(); break;
    case VIEW_EYES_DIZZY:    animDizzyEyes();    break;
    case VIEW_EYES_DEAD:     animDeadEyes();     break;
    case VIEW_EYES_LOOKAROUND: animLookaroundEyes(); break;
    default: animNormalEyes(); break;
  }
}

uint8_t pickRandom() {
  const uint8_t pool[] = {
    VIEW_EYES_NORMAL, VIEW_EYES_SQUISH,
    VIEW_EYES_ANGRY,  VIEW_EYES_SAD,    VIEW_EYES_THINK,
    VIEW_EYES_HAPPY,  VIEW_EYES_ANNOYED,VIEW_EYES_KISS,
    VIEW_EYES_WINK,   VIEW_EYES_BUBBLE, VIEW_EYES_BORED,
    VIEW_EYES_LOOKAROUND
  };
  return pool[random(0, sizeof(pool))];
}

void animLogoReveal() {
  busy = true;
  tft.fillScreen(animBgColor);
  for (uint16_t i = 0; i < LOGO_SEG_COUNT; i++) {
    int16_t x1 = pgm_read_word(&LOGO_SEGS[i][0]);
    int16_t y1 = pgm_read_word(&LOGO_SEGS[i][1]);
    int16_t x2 = pgm_read_word(&LOGO_SEGS[i][2]);
    int16_t y2 = pgm_read_word(&LOGO_SEGS[i][3]);
    tft.drawLine(x1, y1, x2, y2, C_WHITE);
    tft.drawLine(x1 + 1, y1, x2 + 1, y2, C_WHITE);
    if (i % 4 == 0) { server.handleClient(); delay(speedMs(8)); }
  }
  drawLogoFilled(animBgColor, C_WHITE);
  delay(1500);
  busy = false;
}

// ═════════════════════════════════════════════════════════════
//  WEB PAGE
// ═════════════════════════════════════════════════════════════
const char INDEX_HTML[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no">
<title>Clawd Mochi</title>
<style>
*{box-sizing:border-box;margin:0;padding:0;-webkit-tap-highlight-color:transparent}
body{background:#1c1c20;font-family:'Courier New',monospace;color:#e8e4dc;
  display:flex;flex-direction:column;align-items:center;
  padding:20px 14px 52px;gap:14px;min-height:100vh}

.hdr{text-align:center;padding:2px 0 4px}
.mascot{font-size:15px;color:#c96a3e;line-height:1.3;font-weight:bold;
  font-family:'Courier New',monospace;display:block;letter-spacing:1px}
.sitename{font-size:10px;color:#5a5048;margin-top:8px;letter-spacing:3px}

.sec{width:100%;max-width:390px;font-size:10px;color:#8a8278;
  letter-spacing:2px;font-weight:bold;padding:0 2px}

.busy{width:100%;max-width:390px;height:2px;background:#2e2a28;
border-radius:1px;overflow:hidden;opacity:0;transition:opacity .2s}
.busy.show{opacity:1}
.busy-i{height:100%;width:30%;background:#c96a3e;border-radius:1px;
  animation:sl 1s linear infinite}
@keyframes sl{0%{margin-left:-30%}100%{margin-left:100%}}

.ctrl{display:flex;gap:8px;width:100%;max-width:390px}
.cbtn{flex:1;background:#252428;border:1.5px solid #38343a;border-radius:10px;
  color:#b8b4ac;font-family:'Courier New',monospace;font-size:11px;font-weight:bold;
padding:12px 4px;cursor:pointer;text-align:center;transition:all .12s}
.cbtn:active:not(:disabled){transform:scale(.94)}
.cbtn:disabled{opacity:.3;cursor:default}
.cbtn.on{border-color:#c96a3e;color:#c96a3e;background:#201408}
.cbtn.dim{border-color:#2e2a28;color:#4a4540}

.vgrid{display:grid;grid-template-columns:1fr 1fr;gap:8px;width:100%;max-width:390px}
.vbtn{background:#252428;border:1.5px solid #38343a;border-radius:12px;
  color:#d8d4cc;font-family:'Courier New',monospace;
  padding:14px 6px 10px;cursor:pointer;text-align:center;
transition:all .12s;user-select:none}
.vbtn:active:not(:disabled){transform:scale(.94)}
.vbtn:disabled{opacity:.3;cursor:default}
.vbtn .ic{font-size:20px;display:block;margin-bottom:4px;line-height:1;color:#c96a3e}
.vbtn .nm{font-size:12px;font-weight:bold;color:#e8e4dc}
.vbtn .ht{font-size:9px;color:#8a8278;margin-top:3px}
.vbtn.active{border-color:#c96a3e;background:#201408}
.vbtn[data-v="1"].active{border-color:#c96a3e;background:#201408}
.vbtn[data-v="2"].active{border-color:#4a8acd;background:#0c1628}
.vbtn[data-v="3"].active{border-color:#38343a;background:#201c18}

.speed-row{width:100%;max-width:390px;display:flex;align-items:center;gap:10px}
.sl{font-size:10px;color:#6a6058;white-space:nowrap;min-width:36px}
input[type=range]{flex:1;accent-color:#c96a3e;cursor:pointer;height:20px}
.sv{font-size:11px;color:#c96a3e;min-width:44px;text-align:right;font-weight:bold}

.twrap{width:100%;max-width:390px;display:none;flex-direction:column;gap:8px}
.twrap.open{display:flex}
.thdr{display:flex;justify-content:space-between;align-items:center}
.tttl{font-size:11px;color:#28b878;letter-spacing:1px;font-weight:bold}
.tx{background:#0c1e12;border:2px solid #1a4828;border-radius:9px;
  color:#28b878;font-family:'Courier New',monospace;font-size:13px;
  font-weight:bold;padding:10px 18px;cursor:pointer}
.tx:active{background:#081410}
.trow{display:flex;gap:6px}
.tin{flex:1;background:#0c1018;border:1.5px solid #1a2820;border-radius:9px;
  color:#40d880;font-family:'Courier New',monospace;font-size:15px;
padding:11px;outline:none}
.tin::placeholder{color:#2a3828}
.tgo{background:#1a9060;border:none;border-radius:9px;color:#fff;
  font-family:'Courier New',monospace;font-size:22px;font-weight:bold;
  padding:11px 16px;cursor:pointer;min-width:52px}
.tgo:active{background:#0f6040}

.cwrap{width:100%;max-width:390px;background:#222028;border:1.5px solid #38343a;
  border-radius:12px;padding:12px;flex-direction:column;gap:10px;display:none}
.cwrap.open{display:flex}
.crow{display:flex;gap:8px}
.ci{display:flex;flex-direction:column;align-items:center;gap:4px;flex:1}
.cl{font-size:10px;color:#7a7068;letter-spacing:1px;font-weight:bold}
.cs{width:100%;height:38px;border-radius:7px;border:1.5px solid #38343a;cursor:pointer;padding:0}
.dacts{display:flex;gap:7px}
.db{flex:1;background:#1c1820;border:1.5px solid #38343a;border-radius:9px;
  color:#c0bab8;font-family:'Courier New',monospace;font-size:11px;
font-weight:bold;padding:11px 4px;cursor:pointer;transition:all .12s}
.db:active{transform:scale(.95);background:#281838}
.db.hi{border-color:#c96a3e;color:#c96a3e}
canvas{width:100%;border-radius:8px;border:1.5px solid #38343a;
  touch-action:none;cursor:crosshair;display:block}

.toast{position:fixed;bottom:18px;left:50%;transform:translateX(-50%);
  background:#252428;border:1.5px solid #38343a;border-radius:9px;
  font-size:12px;color:#d8d4cc;padding:7px 16px;opacity:0;
transition:opacity .18s;pointer-events:none;white-space:nowrap;z-index:99}
.toast.show{opacity:1}
</style>
</head>
<body>

<div class="hdr">
  <span class="mascot">&#x2590;&#x259B;&#x2588;&#x2588;&#x2588;&#x259C;&#x258C;<br>&#x259C;&#x2588;&#x2588;&#x2588;&#x2588;&#x2588;&#x259B;<br>&#x2598;&#x2598;&nbsp;&#x259D;&#x259D;</span>
  <div class="sitename">CLAWD &middot; MOCHI &middot; CONTROLLER</div>
</div>

<div class="busy" id="busy"><div class="busy-i"></div></div>

<div class="sec">// controls</div>
<div class="ctrl">
  <button class="cbtn on" id="blBtn" onclick="toggleBL()">&#9728; display on</button>
  <button class="cbtn" id="autoBtn" onclick="resumeAuto()">&#8635; auto mode</button>
</div>

<div class="sec">// views</div>
<div class="vgrid">
  <button class="vbtn active" data-v="0" onclick="setView(0)">
    <span class="ic">&#9632; &#9632;</span>
    <span class="nm">Normal eyes</span>
    <span class="ht">wiggle + blink</span>
  </button>
  <button class="vbtn" data-v="1" onclick="setView(1)">
    <span class="ic">&gt; &lt;</span>
    <span class="nm">Squish eyes</span>
    <span class="ht">open / close</span>
  </button>
  <button class="vbtn" data-v="2" onclick="setView(2)">
    <span class="ic">{ }</span>
    <span class="nm">Claude Code</span>
    <span class="ht">opens terminal</span>
  </button>
  <button class="vbtn" data-v="3" onclick="toggleCanvas()">
    <span class="ic">&#11035;</span>
    <span class="nm">Canvas</span>
    <span class="ht">draw on display</span>
  </button>
  <button class="vbtn" data-v="4" onclick="setView(4)">
    <span class="ic">&gt;﹏&lt;</span>
    <span class="nm">Angry</span>
    <span class="ht">shake + brows</span>
  </button>
  <button class="vbtn" data-v="5" onclick="setView(5)">
    <span class="ic">╥_╥</span>
    <span class="nm">Sad</span>
    <span class="ht">tears drop</span>
  </button>
  <button class="vbtn" data-v="6" onclick="setView(6)">
    <span class="ic">_＿_</span>
    <span class="nm">Tired</span>
    <span class="ht">eyes droop</span>
  </button>
  <button class="vbtn" data-v="7" onclick="setView(7)">
    <span class="ic">－ －</span>
    <span class="nm">Sleep</span>
    <span class="ht">zzz float up</span>
  </button>
  <button class="vbtn" data-v="8" onclick="setView(8)">
    <span class="ic">O ．</span>
    <span class="nm">Think</span>
    <span class="ht">eyes drift...</span>
  </button>
  <button class="vbtn" data-v="9" onclick="setView(9)">
    <span class="ic">^ ^</span>
    <span class="nm">Happy</span>
    <span class="ht">Happy</span>
  </button>
  <button class="vbtn" data-v="10" onclick="setView(10)">
    <span class="ic">¬ ¬</span>
    <span class="nm">Annoyed</span>
    <span class="ht">Annoyed</span>
  </button>
  <button class="vbtn" data-v="11" onclick="setView(11)">
    <span class="ic">| ♡</span>
    <span class="nm">Kiss</span>
    <span class="ht">wink + heart</span>
  </button>
  <button class="vbtn" data-v="12" onclick="setView(12)">
    <span class="ic">– –</span>
    <span class="nm">眨眼</span>
    <span class="ht">blink blink</span>
  </button>
  <button class="vbtn" data-v="14" onclick="setView(14)">
    <span class="ic">_ _</span>
    <span class="nm">Bored</span>
    <span class="ht">Bored</span>
  </button>
  <button class="vbtn" data-v="15" onclick="setView(15)">
    <span class="ic">| |?</span>
    <span class="nm">疑问</span>
    <span class="ht">eyes + ?</span>
  </button>
  <button class="vbtn" data-v="16" onclick="setView(16)">
    <span class="ic">@ @</span>
    <span class="nm">晕</span>
    <span class="ht">dizzy spin</span>
  </button>
  <button class="vbtn" data-v="17" onclick="setView(17)">
    <span class="ic">X X</span>
    <span class="nm">死掉</span>
    <span class="ht">flash + shake</span>
  </button>
  <button class="vbtn" data-v="18" onclick="setView(18)">
    <span class="ic">◄ ►</span>
    <span class="nm">左顾右盼</span>
    <span class="ht">look around</span>
  </button>
  <input type="range" id="spd" min="1" max="3" value="1" step="1" oninput="setSpeed(this.value)">
  <span class="sv" id="spdV">slow</span>
</div>

<div class="ctrl">
  <div class="ci" style="flex:1;display:flex;flex-direction:column;gap:4px;align-items:stretch">
    <span class="cl" style="font-size:10px;color:#8a8278;letter-spacing:1px;font-weight:bold;text-align:center">BACKGROUND</span>
    <input type="color" class="cs" id="bgCol" value="#aa4818" oninput="onBgChange(this.value)">
  </div>
  <div class="ci" style="flex:1;display:flex;flex-direction:column;gap:4px;align-items:stretch">
    <span class="cl" style="font-size:10px;color:#8a8278;letter-spacing:1px;font-weight:bold;text-align:center">PEN COLOR</span>
    <input type="color" class="cs" id="penCol" value="#000000">
  </div>
</div>

<div class="sec">// terminal</div>
<div class="twrap" id="twrap">
  <div class="thdr">
    <span class="tttl">&#9658; clawd:~$</span>
    <button class="tx" onclick="closeTerm()">&#x2715; exit terminal</button>
  </div>
  <div class="trow">
    <input class="tin" id="tin" type="text" placeholder="type here..."
           autocomplete="off" autocorrect="off" autocapitalize="off" spellcheck="false">
    <button class="tgo" onclick="termEnter()">&#8629;</button>
  </div>
</div>

<div class="cwrap" id="cwrap">
  <div class="dacts">
    <button class="db hi" onclick="clearAll()">&#11035; clear</button>
    <button class="db" style="border-color:#28b878;color:#28b878" onclick="toggleCanvas()">&#10003; done</button>
  </div>
  <canvas id="cvs" width="240" height="240"></canvas>
</div>

<div class="toast" id="toast"></div>

<script>
let activeView  = 0;
let termOpen    = false;
let canvasOpen  = false;
let blOn        = true;
let isBusy      = false;
let drawing     = false;
let lastX = 0, lastY = 0;
let tt;
const spdLabels = ['','slow','normal','fast'];

// 这里补全了所有的键盘映射 (17个按键)
const keys = ['w','s','d','x','e','f','g','h','i','j','k','l','m','','n','o','p','u','b'];

function toast(msg, ok=true) {
  const el = document.getElementById('toast');
  el.textContent = msg;
  el.style.borderColor = ok ? '#28b878' : '#c96a3e';
  el.classList.add('show');
  clearTimeout(tt);
  tt = setTimeout(() => el.classList.remove('show'), 1300);
}

function setBusy(b) {
  isBusy = b;
  document.getElementById('busy').classList.toggle('show', b);
  const locked = b || termOpen;
  document.querySelectorAll('.vbtn').forEach(el => {
    el.disabled = canvasOpen ? parseInt(el.dataset.v) !== 3 : locked;
  });
  document.querySelectorAll('.lbtn').forEach(el => el.disabled = locked || canvasOpen);
  document.querySelectorAll('.cbtn').forEach(el => {
    if (el.id !== 'blBtn') el.disabled = locked;
  });
}

async function req(path) {
  try { const r = await fetch(path); return r.ok; }
  catch(e) { toast('no connection', false); return false; }
}

async function waitNotBusy() {
  for (let i = 0; i < 100; i++) {
    try {
      const r = await fetch('/state');
      const j = await r.json();
      if (!j.busy) return;
    } catch(e) {}
    await new Promise(r => setTimeout(r, 150));
  }
}

async function onBgChange(hex) {
  if (canvasOpen) {
    await req('/draw/clear?bg=' + encodeURIComponent(hex));
  } else {
    await req('/redraw?bg=' + encodeURIComponent(hex));
  }
  redrawCanvas(hex);
}

async function setSpeed(v) {
  document.getElementById('spdV').textContent = spdLabels[v];
  await req('/speed?v=' + v);
}

async function setView(v) {
  if (isBusy || termOpen || canvasOpen) return;
  if (v === 3) { toggleCanvas(); return; } 
  if (!await req('/cmd?k=' + keys[v])) return;
  activeView = v;
  document.querySelectorAll('.vbtn').forEach(b =>
    b.classList.toggle('active', parseInt(b.dataset.v) === v));
  if (v === 2) {
    termOpen = true;
    document.getElementById('twrap').classList.add('open');
    setBusy(false);
    document.querySelectorAll('.vbtn,.lbtn').forEach(b => b.disabled = true);
    const cvb = document.getElementById('cvBtn');
    if (cvb) cvb.disabled = true;
    document.getElementById('tin').focus();
    toast('terminal open');
    return;
  }
  setBusy(true);
  await waitNotBusy();
  setBusy(false);
}

async function resumeAuto() {
  await req('/cmd?k=r');
  toast('auto mode resumed');
}

async function toggleBL() {
  blOn = !blOn;
  await req('/backlight?on=' + (blOn ? 1 : 0));
  const b = document.getElementById('blBtn');
  b.textContent = blOn ? '\u2600 display on' : '\u25cb display off';
  b.classList.toggle('on', blOn);
  b.classList.toggle('dim', !blOn);
}

async function toggleCanvas() {
  canvasOpen = !canvasOpen;
  document.getElementById('cwrap').classList.toggle('open', canvasOpen);
  const b = document.getElementById('cvBtn');
  if (b) { b.classList.toggle('on', canvasOpen); b.textContent = canvasOpen ? '\u2b1b canvas on' : '\u2b1b canvas'; }
  document.querySelectorAll('.vbtn').forEach(btn =>
    btn.classList.toggle('active', canvasOpen && parseInt(btn.dataset.v) === 3));
  await req('/canvas?on=' + (canvasOpen ? 1 : 0));
  if (canvasOpen) {
    const bg = document.getElementById('bgCol').value;
    redrawCanvas(bg);
    await req('/draw/clear?bg=' + encodeURIComponent(bg));
    document.querySelectorAll('.vbtn,.lbtn').forEach(b => b.disabled = true);
    toast('canvas active');
  } else {
    setBusy(false);
    toast('canvas off');
  }
}

const tin = document.getElementById('tin');
let lastVal = '';
tin.addEventListener('input', async () => {
  const cur = tin.value, prev = lastVal;
  if (cur.length > prev.length) {
    await req('/char?c=' + encodeURIComponent(cur[cur.length - 1]));
  } else if (cur.length < prev.length) {
    await req('/char?c=%08');
  }
  lastVal = cur;
});
async function termEnter() {
  await req('/char?c=%0A');
  tin.value = ''; lastVal = ''; tin.focus();
}
tin.addEventListener('keydown', e => {
  if (e.key === 'Enter') { e.preventDefault(); termEnter(); }
});
async function closeTerm() {
  await req('/cmd?k=q');
  termOpen = false;
  document.getElementById('twrap').classList.remove('open');
  setBusy(false);
  toast('terminal closed');
}

const cvs = document.getElementById('cvs');
const ctx = cvs.getContext('2d');
let strokePts = [];

function getPos(e) {
  const r = cvs.getBoundingClientRect();
  const sx = cvs.width / r.width, sy = cvs.height / r.height;
  const s = e.touches ? e.touches[0] : e;
  return { x: (s.clientX - r.left) * sx, y: (s.clientY - r.top) * sy };
}

function redrawCanvas(hex) {
  ctx.fillStyle = hex;
  ctx.fillRect(0, 0, cvs.width, cvs.height);
}

function startDraw(e) {
  e.preventDefault();
  drawing = true;
  strokePts = [];
  const p = getPos(e); lastX = p.x; lastY = p.y;
  strokePts.push({ x: Math.round(p.x), y: Math.round(p.y) });
  ctx.beginPath();
  ctx.arc(p.x, p.y, 2, 0, Math.PI * 2);
  ctx.fillStyle = document.getElementById('penCol').value; ctx.fill();
}
function moveDraw(e) {
  if (!drawing) return; e.preventDefault();
  const p = getPos(e);
  ctx.beginPath(); ctx.moveTo(lastX, lastY); ctx.lineTo(p.x, p.y);
  ctx.strokeStyle = document.getElementById('penCol').value;
  ctx.lineWidth = 4; ctx.lineCap = 'round'; ctx.stroke();
  strokePts.push({ x: Math.round(p.x), y: Math.round(p.y) });
  lastX = p.x; lastY = p.y;
}
async function endDraw(e) {
  if (!drawing) return;
  drawing = false;
  if (!canvasOpen || strokePts.length < 1) return;
  const pen = document.getElementById('penCol').value.replace('#', '');
  const pts = strokePts.map(p => p.x + ',' + p.y).join(';');
  await req('/draw/stroke?pen=' + pen + '&pts=' + encodeURIComponent(pts));
  strokePts = [];
}

cvs.addEventListener('mousedown',  startDraw);
cvs.addEventListener('mousemove',  moveDraw);
cvs.addEventListener('mouseup',    endDraw);
cvs.addEventListener('mouseleave', endDraw);
cvs.addEventListener('touchstart', startDraw, {passive:false});
cvs.addEventListener('touchmove',  moveDraw,  {passive:false});
cvs.addEventListener('touchend',   endDraw);

async function clearAll() {
  const bg = document.getElementById('bgCol').value;
  redrawCanvas(bg);
  await req('/draw/clear?bg=' + encodeURIComponent(bg));
  toast('cleared');
}

(async () => {
  try {
    const r = await fetch('/state');
    const j = await r.json();
    const spd = j.speed || 1;
    document.getElementById('spd').value = spd;
    document.getElementById('spdV').textContent = spdLabels[spd];
    if (j.bl === false) {
      blOn = false;
      const b = document.getElementById('blBtn');
      b.textContent = '\u25cb display off';
      b.classList.remove('on'); b.classList.add('dim');
    }
  } catch(e) {}
  document.getElementById('bgCol').value = '#aa4818';
  redrawCanvas('#aa4818');
})();
</script>
</body>
</html>
)rawhtml";

// ═════════════════════════════════════════════════════════════
//  WEB ROUTES
// ═════════════════════════════════════════════════════════════

void routeRoot() {
  server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.send_P(200, "text/html", INDEX_HTML);
}

void routeCmd() {
  if (!server.hasArg("k") || server.arg("k").isEmpty()) {
    server.send(400, "application/json", "{\"e\":1}"); return;
  }
  const char c = server.arg("k")[0];

  if (termMode) {
    if (c == 'q') { termMode = false; drawCodeView(); }
    server.send(200, "application/json", "{\"ok\":1}"); return;
  }

  server.send(200, "application/json", "{\"ok\":1}");
  if (c != 'r') { manualOverride = true; overrideEnd = millis() + 600000UL; }
  switch (c) {
    case 'w': currentView = VIEW_EYES_NORMAL;  animNormalEyes();  break;
    case 's': currentView = VIEW_EYES_SQUISH;  animSquishEyes();  break;
    case 'd':
      currentView = VIEW_CODE; drawCodeView();
      termMode = true; termClear(); termFullRedraw(); break;
    case 'a':
      currentView = VIEW_EYES_NORMAL;
      animLogoReveal();
      break;
    case 'e': currentView = VIEW_EYES_ANGRY;   animAngryEyes();   break;
    case 'f': currentView = VIEW_EYES_SAD;     animSadEyes();     break;
    case 'g': currentView = VIEW_EYES_TIRED;   animTiredEyes();   break;
    case 'h': currentView = VIEW_EYES_SLEEP;   animSleepEyes();   break;
    case 'i': currentView = VIEW_EYES_THINK;   animThinkEyes();   break;
    case 'j': currentView = VIEW_EYES_HAPPY;   animHappyEyes();   break;
    case 'k': currentView = VIEW_EYES_ANNOYED; animAnnoyedEyes(); break;
    case 'l': currentView = VIEW_EYES_KISS;    animKissEyes();    break;
    case 'm': currentView = VIEW_EYES_WINK;     animWinkEyes();     break;
    case 'n': currentView = VIEW_EYES_BORED;    animBoredEyes();    break;
    case 'o': currentView = VIEW_EYES_CONFUSED; animConfusedEyes(); break;
    case 'p': currentView = VIEW_EYES_DIZZY;    animDizzyEyes();    break;
    case 'u': currentView = VIEW_EYES_DEAD;     animDeadEyes();     break;
    case 'b': currentView = VIEW_EYES_LOOKAROUND; animLookaroundEyes(); break;
    case 'r': manualOverride = false; break;
  }
}

void routeChar() {
  if (!termMode) { server.send(200, "application/json", "{\"ok\":1}"); return; }
  const String val = server.arg("c");
  if (val.length() > 0) termAddChar(val[0]);
  server.send(200, "application/json", "{\"ok\":1}");
}

void routeSpeed() {
  if (server.hasArg("v")) animSpeed = constrain(server.arg("v").toInt(), 1, 3);
  server.send(200, "application/json", "{\"ok\":1}");
}

void routeRedraw() {
  if (server.hasArg("bg")) {
    animBgColor = hexToRgb565(server.arg("bg"));
    drawBgColor = animBgColor;
  }
  switch (currentView) {
    case VIEW_EYES_NORMAL:   drawNormalEyes();        break;
    case VIEW_EYES_SQUISH:   drawSquishEyes();        break;
    case VIEW_CODE:          drawCodeView();          break;
    case VIEW_DRAW:          tft.fillScreen(drawBgColor); break;
    case VIEW_EYES_ANGRY:    drawAngryEyes();         break;
    case VIEW_EYES_SAD:      drawSadEyes();           break;
    case VIEW_EYES_TIRED:    drawTiredEyes(30);       break;
    case VIEW_EYES_SLEEP:    drawSleepEyes(3);        break;
    case VIEW_EYES_THINK:    drawThinkEyes();         break;
    case VIEW_EYES_HAPPY:    drawHappyEyes();         break;
    case VIEW_EYES_ANNOYED:  drawAnnoyedEyes();       break;
    case VIEW_EYES_KISS:     drawKissEyes(0, 12);     break;
    case VIEW_EYES_WINK:     drawWinkEyes();          break;
    case VIEW_EYES_BORED:    drawBoredEyes();       break;
    case VIEW_EYES_CONFUSED: drawConfusedEyes(0,true); break;
    case VIEW_EYES_DIZZY:    drawDizzyEyes(0);        break;
    case VIEW_EYES_DEAD:     drawDeadEyes();          break;
    case VIEW_EYES_LOOKAROUND: drawNormalEyes();        break;
  }
  server.send(200, "application/json", "{\"ok\":1}");
}

void routeCanvas() {
  const bool on = server.hasArg("on") && server.arg("on") == "1";
  if (on) { currentView = VIEW_DRAW; tft.fillScreen(drawBgColor); }
  server.send(200, "application/json", "{\"ok\":1}");
}

void routeDrawClear() {
  const String bg = server.hasArg("bg") ? server.arg("bg") : "#aa4818";
  drawBgColor = hexToRgb565(bg);
  animBgColor = drawBgColor;
  currentView = VIEW_DRAW; termMode = false;
  tft.fillScreen(drawBgColor);
  server.send(200, "application/json", "{\"ok\":1}");
}

void routeDrawStroke() {
  if (!server.hasArg("pts") || !server.hasArg("pen")) {
    server.send(200, "application/json", "{\"ok\":1}"); return;
  }
  const uint16_t color = hexToRgb565(server.arg("pen"));
  const String   data  = server.arg("pts");
  currentView = VIEW_DRAW;
  struct Pt { int16_t x, y; };
  Pt prev = {-1, -1};
  int start = 0;
  while (start < (int)data.length()) {
    int semi = data.indexOf(';', start);
    if (semi == -1) semi = data.length();
    String entry = data.substring(start, semi);
    const int comma = entry.indexOf(',');
    if (comma > 0) {
      const int16_t x = entry.substring(0, comma).toInt();
      const int16_t y = entry.substring(comma + 1).toInt();
      if (prev.x >= 0) {
        tft.drawLine(prev.x, prev.y, x, y, color);
        tft.drawLine(prev.x + 1, prev.y, x + 1, y, color);
        tft.drawLine(prev.x, prev.y + 1, x, y + 1, color);
      } else {
        tft.fillCircle(x, y, 2, color);
      }
      prev = {x, y};
    }
    start = semi + 1;
  }
  server.send(200, "application/json", "{\"ok\":1}");
}

void routeBacklight() {
  setBacklight(server.hasArg("on") && server.arg("on") == "1");
  server.send(200, "application/json", "{\"ok\":1}");
}

String rgb565ToHex(uint16_t c) {
  uint8_t r = ((c >> 11) & 0x1F) << 3;
  uint8_t g = ((c >> 5)  & 0x3F) << 2;
  uint8_t b = (c & 0x1F) << 3;
  char buf[8];
  snprintf(buf, sizeof(buf), "#%02x%02x%02x", r, g, b);
  return String(buf);
}

void routeState() {
  String j = "{\"view\":";
  j += currentView;
  j += ",\"busy\":";   j += busy        ? "true" : "false";
  j += ",\"term\":";   j += termMode    ? "true" : "false";
  j += ",\"bl\":";     j += backlightOn ? "true" : "false";
  j += ",\"speed\":";  j += animSpeed;
  j += "}";
  server.send(200, "application/json", j);
}

void routeNotFound() { server.send(404, "text/plain", "not found"); }

// ═════════════════════════════════════════════════════════════
//  SETUP
// ═════════════════════════════════════════════════════════════

void setup() {
  Serial.begin(115200);

  pinMode(TFT_BLK, OUTPUT);
  pinMode(TOUCH_PIN, INPUT);
  setBacklight(true);
  SPI.begin(8, -1, 10, TFT_CS);   // SCK=8, MOSI=10
  tft.init(240, 240);
  tft.setSPISpeed(40000000);
  tft.setRotation(1);
  initColours();

  tft.fillScreen(animBgColor);
  tft.setTextColor(C_WHITE); tft.setTextSize(3);
  tft.setCursor(DISP_W / 2 - 54, DISP_H / 2 - 22); tft.print("Clawd");
  tft.setCursor(DISP_W / 2 - 54, DISP_H / 2 + 14); tft.print("Mochi");
  delay(1200);
  animLogoReveal();

  tft.fillScreen(C_DARKBG);
  tft.setTextColor(C_WHITE);
  tft.setTextSize(2);
  tft.setCursor(12, 80); tft.print("Connecting...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  uint8_t tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 24) {
    delay(500); tries++;
    server.handleClient();
  }

  if (WiFi.status() == WL_CONNECTED) {
    configTime(8 * 3600, 0, "pool.ntp.org", "time.cloudflare.com");
    struct tm ti;
    for (uint8_t i = 0; i < 10; i++) { if (getLocalTime(&ti)) break; delay(500); }
    tft.fillScreen(C_DARKBG);
    tft.fillRect(0, 0, DISP_W, 4, C_ORANGE);
    tft.setTextColor(C_WHITE);  tft.setTextSize(2);
    tft.setCursor(12, 16);  tft.print("Connected!");
    tft.setTextColor(C_MUTED);  tft.setTextSize(1);
    tft.setCursor(12, 46);  tft.print("IP address:");
    tft.setTextColor(C_ORANGE); tft.setTextSize(2);
    tft.setCursor(12, 62);
    tft.print(WiFi.localIP().toString());
    tft.setTextColor(C_MUTED);  tft.setTextSize(1);
    tft.setCursor(12, 92);  tft.print("Open browser at IP above");
    tft.setCursor(12, 110); tft.print("Auto mode: ON");
    delay(3000);
  } else {
    WiFi.mode(WIFI_AP);
    WiFi.softAP("ClaWD-Mochi", "clawd1234");
    tft.fillScreen(C_DARKBG);
    tft.fillRect(0, 0, DISP_W, 4, C_ORANGE);
    tft.setTextColor(C_ORANGE); tft.setTextSize(2);
    tft.setCursor(12, 16); tft.print("WiFi failed");
    tft.setTextColor(C_MUTED);  tft.setTextSize(1);
    tft.setCursor(12, 46); tft.print("Hotspot: ClaWD-Mochi");
    tft.setCursor(12, 62);
    tft.print("pw: clawd1234");
    tft.setCursor(12, 78); tft.print("192.168.4.1");
    delay(3000);
  }

  server.on("/",            HTTP_GET, routeRoot);
  server.on("/cmd",         HTTP_GET, routeCmd);
  server.on("/char",        HTTP_GET, routeChar);
  server.on("/speed",       HTTP_GET, routeSpeed);
  server.on("/redraw",      HTTP_GET, routeRedraw);
  server.on("/canvas",      HTTP_GET, routeCanvas);
  server.on("/draw/clear",  HTTP_GET, routeDrawClear);
  server.on("/draw/stroke", HTTP_GET, routeDrawStroke);
  server.on("/backlight",   HTTP_GET, routeBacklight);
  server.on("/state",       HTTP_GET, routeState);
  server.onNotFound(routeNotFound);
  server.begin();

  lastExprMs  = millis();
  nextExprMs  = 10000;
}

// ═════════════════════════════════════════════════════════════
//  LOOP
// ═════════════════════════════════════════════════════════════

void loop() {
  // 1. 优先处理网页请求，防止 HTML 卡死
  server.handleClient();
  if (busy) return;

  unsigned long now = millis();

  // ── Touch sensor (TTP223 点动模式) ────────────────────────────
  // 摸一下 → 随机表情，25秒内摸5次以上一直晕，最后一次触摸后20s回到时间逻辑
  bool touchNow = (bool)digitalRead(TOUCH_PIN);
  // 处理动画被触摸打断的情况
  if (touchInterrupt) {
    touchInterrupt = false;
    lastTouchMs = now;
    if (touchCount == 0) firstTouchMs = now;
    touchCount++;
    if (touchCount >= TOUCH_OVERLOAD) touchOverload = true;
    uint8_t chosen;
    if (touchOverload) {
      chosen = VIEW_EYES_DIZZY;
    } else {
      const uint8_t touchExprs[] = { VIEW_EYES_HAPPY, VIEW_EYES_KISS, VIEW_EYES_SQUISH, VIEW_EYES_WINK };
      chosen = touchExprs[random(0, 4)];
    }
    runExpr(chosen);
    exprPlayCount = 0;
    lastExprMs = now;
    nextExprMs = 20000UL;
    lastTouchState = touchNow;
    return;
  }
  // Reset overload if no touch for 20 seconds
  if (touchOverload && (now - lastTouchMs > TOUCH_RESET_MS)) {
    touchOverload = false;
    touchCount = 0;
  }
  // Reset burst window if first touch was >25s ago and not overloaded
  if (!touchOverload && touchCount > 0 && (now - firstTouchMs > TOUCH_WINDOW_MS)) {
    touchCount = 0;
  }
  touchInterrupt = false;  // clear flag after processing
  if (touchNow && !lastTouchState && (now - lastTouchMs > TOUCH_DEBOUNCE_MS)) {
    lastTouchMs = now;
    // Track touch count within window
    if (touchCount == 0) {
      firstTouchMs = now;
    }
    touchCount++;
    if (touchCount >= TOUCH_OVERLOAD) touchOverload = true;

    uint8_t chosen;
    if (touchOverload) {
      chosen = VIEW_EYES_DIZZY;
    } else {
      const uint8_t touchExprs[] = { VIEW_EYES_HAPPY, VIEW_EYES_KISS, VIEW_EYES_SQUISH, VIEW_EYES_WINK };
      chosen = touchExprs[random(0, 4)];
    }
    runExpr(chosen);
    exprPlayCount = 0;  // reset play count on touch
    // 每次触摸都重置20秒冷却，最后一次触摸后20s回到时间逻辑
    lastExprMs = now;
    nextExprMs = 20000UL;
  }
  lastTouchState = touchNow;

  // 2. 手动模式：20秒过期后自动回到时间逻辑内的表情
  if (manualOverride) {
    if (now - (overrideEnd - 600000UL) >= 20000UL) {
      manualOverride = false;
      int8_t h = getHour();
      if (h >= 23 || h < 8) {
        // 不用runExpr避免阻塞，直接设置状态让轮播接管
        currentView = VIEW_EYES_SLEEP;
        drawSleepEyes(3);
        lastExprMs = now;
        nextExprMs = 8000;
        exprPlayCount = 0;
        return;
      }
      if (h >= 12 && h < 13) {
        currentView = VIEW_EYES_TIRED;
        drawTiredEyes(30);
        lastExprMs = now;
        nextExprMs = 15000;
        exprPlayCount = 0;
        return;
      }
      lastExprMs = now;
      nextExprMs = 3000;
      exprPlayCount = 0;
      return;
    }
    static unsigned long lastBlinkMs = 0;
    if (currentView == VIEW_EYES_NORMAL && (now - lastBlinkMs > 4000)) {
      animNormalEyes();
      lastBlinkMs = now;
    }
    return;
  }

  // 3. 自动轮巡冷却时间检查
  if (now - lastExprMs < nextExprMs) return;

  // 基于特定时间的强规则（持续播放，不切换）
  int8_t h = getHour();
  // 23:00 - 08:00 → 睡觉持续播放
  if (h >= 23 || h < 8) {
    currentView = VIEW_EYES_SLEEP;
    animSleepEyes();  // 这里用anim因为是正常轮播周期，不是切换
    lastExprMs = now;
    nextExprMs = 8000;
    return;
  }
  // 12:00 - 13:00 → 困持续播放
  if (h >= 12 && h < 13) {
    currentView = VIEW_EYES_TIRED;
    animTiredEyes();
    lastExprMs = now;
    nextExprMs = 15000;
    return;
  }

  // 4. 同一表情连播 EXPR_REPEAT 次后再切换
  if (exprPlayCount < EXPR_REPEAT) {
    runExpr(currentView);          // 重播当前表情
    exprPlayCount++;
    lastExprMs = now;
    nextExprMs = 2000;
    return;
  }

  // 5. 连播次数用完，随机挑一个新表情
  exprPlayCount = 0;
  uint8_t choice = random(0, 100);
  uint8_t nextView = VIEW_EYES_NORMAL;

  if (choice < 19)       nextView = VIEW_EYES_NORMAL;    // 19%
  else if (choice < 28)  nextView = VIEW_EYES_HAPPY;     //  9%
  else if (choice < 36)  nextView = VIEW_EYES_BORED;     //  8%
  else if (choice < 44)  nextView = VIEW_EYES_SQUISH;    //  8%
  else if (choice < 52)  nextView = VIEW_EYES_THINK;     //  8%
  else if (choice < 60)  nextView = VIEW_EYES_ANNOYED;   //  8%
  else if (choice < 67)  nextView = VIEW_EYES_ANGRY;     //  7%
  else if (choice < 74)  nextView = VIEW_EYES_SAD;       //  7%
  else if (choice < 79)  nextView = VIEW_EYES_WINK;      //  5%
  else if (choice < 84)  nextView = VIEW_EYES_KISS;      //  5%
  else if (choice < 87)  nextView = VIEW_EYES_LOOKAROUND;//  3%
  else if (choice < 90)  nextView = VIEW_EYES_SLEEP;     //  3%
  else if (choice < 92)  nextView = VIEW_EYES_TIRED;     //  2%
  else if (choice < 94)  nextView = VIEW_EYES_CONFUSED;  //  2%
  else if (choice < 96)  nextView = VIEW_EYES_DIZZY;     //  2%
  else                   nextView = VIEW_EYES_DEAD;      //  2%

  // 触发选中的新表情（第一次播放）
  runExpr(nextView);
  exprPlayCount = 1;
  lastExprMs = now;
  nextExprMs = random(6000, 10000);
}