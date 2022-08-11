#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"

#define TFT_DC 9
#define TFT_CS -1
#define TFT_RST 8

Adafruit_ILI9341 tft = Adafruit_ILI9341(&SPI, TFT_DC, TFT_CS, TFT_RST);

void setup() {
  Serial.begin(115200);
  Serial.println("ILI9341 Test!"); 
 
  tft.begin(40000000);
  /*
  // read diagnostics (optional but can help debug problems)
  uint8_t x = tft.readcommand8(ILI9341_RDMODE);
  Serial.print("Display Power Mode: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDMADCTL);
  Serial.print("MADCTL Mode: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDPIXFMT);
  Serial.print("Pixel Format: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDIMGFMT);
  Serial.print("Image Format: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDSELFDIAG);
  Serial.print("Self Diagnostic: 0x"); Serial.println(x, HEX); 
  
  Serial.println(F("Benchmark                Time (microseconds)"));
  */
  print_board();
}


void loop(void) {
  
}

#define HW 8
#define CELL_SIZE 25
#define BOARD_SIZE (CELL_SIZE * HW)
#define BOARD_SX 20
#define BOARD_SY 20
#define DOT_RADIUS 2


void print_board(){
  tft.fillScreen(ILI9341_DARKGREEN);
  int i, x, y;
  for (i = 0; i < HW + 1; ++i){
    x = BOARD_SX + i * CELL_SIZE;
    y = BOARD_SY;
    tft.drawLine(x, y, x, y + BOARD_SIZE, ILI9341_BLACK);
    x = BOARD_SX;
    y = BOARD_SY + i * CELL_SIZE;
    tft.drawLine(x, y, x + BOARD_SIZE, y, ILI9341_BLACK);
  }
  tft.fillCircle(BOARD_SX + CELL_SIZE * 2, BOARD_SY + CELL_SIZE * 2, DOT_RADIUS, ILI9341_BLACK);
  tft.fillCircle(BOARD_SX + CELL_SIZE * 6, BOARD_SY + CELL_SIZE * 2, DOT_RADIUS, ILI9341_BLACK);
  tft.fillCircle(BOARD_SX + CELL_SIZE * 2, BOARD_SY + CELL_SIZE * 6, DOT_RADIUS, ILI9341_BLACK);
  tft.fillCircle(BOARD_SX + CELL_SIZE * 6, BOARD_SY + CELL_SIZE * 6, DOT_RADIUS, ILI9341_BLACK);
  tft.setTextColor(ILI9341_BLACK);
  tft.setTextSize(1);
  for (i = 0; i < HW; ++i){
    tft.setCursor(30 + i * CELL_SIZE, 10);
    tft.print((char)('A' + i));
  }
  for (i = 0; i < HW; ++i){
    tft.setCursor(10, 30 + CELL_SIZE * i);
    tft.print((char)('1' + i));
  }
}

void print_discs(){
  
}
