#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Preferences.h> 

// --- PIN DEFINITIONS ---
#define TFT_CS    5
#define TFT_DC    2
#define TFT_RST   4
#define TFT_BL    21

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
Preferences prefs;

// --- CALIBRATION CONSTANTS ---
float k_factor = 12.40; 
float diode_vf = 0.17; 
float g_pFwd = 0, g_pRef = 0, g_swr = 1.0, g_vFwd = 0, g_vRef = 0;

// --- PEAK HOLD SETTINGS ---
float peakPower = 0;
float peakSWR_hold = 1.0;
unsigned long peakTime = 0;
const unsigned long PEAK_HOLD_MS = 3000; 
bool alertActive = false;

void setup() {
  // Initialize Backlight
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  
  // Load Calibration from Flash Memory
  prefs.begin("rf-cal", false);
  k_factor = prefs.getFloat("k", 12.40);
  diode_vf = prefs.getFloat("v", 0.17);

  // Initialize Display
  tft.init(240, 320, SPI_MODE0);
  tft.setRotation(1); 
  tft.invertDisplay(false); 
  tft.fillScreen(ST77XX_BLACK);

  // Static Header
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.setTextSize(2);
  tft.setCursor(60, 5);
  tft.print("RF STATION MONITOR");
}

void loop() {
  // 1. ADC SAMPLING & AVERAGING
  long sumF = 0, sumR = 0;
  for(int i = 0; i < 60; i++) {
    sumF += analogRead(34);
    sumR += analogRead(35);
    delayMicroseconds(50);
  }
  
  // 2. VOLTAGE CONVERSION
  g_vFwd = (sumF / 60.0 / 4095.0) * 3.3;
  g_vRef = (sumR / 60.0 / 4095.0) * 3.3;

  // 3. DIODE COMPENSATION
  float compF = (g_vFwd > 0.01) ? g_vFwd + diode_vf : 0;
  float compR = (g_vRef > 0.01) ? g_vRef + diode_vf : 0;

  // 4. POWER CALCULATION (P = K * V^2)
  g_pFwd = (g_vFwd > 0.02) ? k_factor * sq(compF) : 0;
  g_pRef = (g_vRef > 0.02) ? k_factor * sq(compR) : 0;

  // 5. SWR CALCULATION
  if (g_vFwd > 0.1) {
    float gamma = compR / compF;
    if (gamma > 0.95) g_swr = 9.99;
    else g_swr = (1.0 + gamma) / (1.0 - gamma);
  } else { 
    g_swr = 1.0; 
  }

  // 6. PEAK HOLD LOGIC
  if (g_pFwd >= peakPower) {
    peakPower = g_pFwd;
    if (g_swr > peakSWR_hold) peakSWR_hold = g_swr;
    peakTime = millis();
  } else if (millis() - peakTime > PEAK_HOLD_MS) {
    peakPower = g_pFwd;
    peakSWR_hold = g_swr;
  }

  updateDisplay();
}

void updateDisplay() {
  // HIGH SWR ALERT SCREEN
  if (peakSWR_hold >= 3.0 && g_pFwd > 1.0) {
    if (!alertActive) { 
      tft.fillScreen(ST77XX_RED); 
      alertActive = true; 
    }
    tft.setTextColor(ST77XX_WHITE, ST77XX_RED);
    tft.setTextSize(5); 
    tft.setCursor(45, 80); 
    tft.print("HIGH SWR");
    
    tft.setTextSize(4);
    tft.setCursor(80, 150);
    tft.printf("SWR: %1.2f", peakSWR_hold);
    return; 
  }

  // RESET TO NORMAL SCREEN AFTER ALERT
  if (alertActive && peakSWR_hold < 3.0) {
    tft.fillScreen(ST77XX_BLACK);
    tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    tft.setTextSize(2);
    tft.setCursor(60, 5);
    tft.print("RF STATION MONITOR");
    alertActive = false;
  }

  // FWD POWER
  tft.setTextColor(ST77XX_GREEN, ST77XX_BLACK);
  tft.setTextSize(3); tft.setCursor(10, 35); tft.print("FWD:");
  tft.setTextSize(4); tft.setCursor(95, 30); 
  tft.printf("%5.1f W ", g_pFwd); // Padded to overwrite old "W"

  // PEAK POWER
  tft.setTextSize(3); tft.setCursor(10, 70); tft.print("PEAK:");
  tft.setTextSize(4); tft.setCursor(115, 65); 
  tft.printf("%5.1f W ", peakPower);

  // REVERSE POWER
  tft.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
  tft.setTextSize(2); tft.setCursor(10, 105); tft.print("REV:");
  tft.setTextSize(3); tft.setCursor(75, 100); 
  tft.printf("%4.1f W ", g_pRef);

  // LIVE SWR
  tft.setTextColor(ST77XX_RED, ST77XX_BLACK);
  tft.setTextSize(3); tft.setCursor(10, 140); tft.print("SWR:");
  tft.setTextSize(6); tft.setCursor(95, 135);
  if (g_pFwd < 0.4) tft.print("---   ");
  else tft.printf("%1.2f ", g_swr);

  // PEAK SWR
  tft.setTextSize(2); tft.setCursor(10, 185);
  tft.setTextColor(ST77XX_RED, ST77XX_BLACK);
  tft.print("SWR PEAK:");
  tft.setCursor(130, 185);
  if (g_pFwd < 0.4) tft.print("---   ");
  else tft.printf("%1.2f ", peakSWR_hold);

  // ANALOG VOLTAGES (DEBUG DATA)
  tft.setTextColor(ST77XX_YELLOW, ST77XX_BLACK);
  tft.setTextSize(2); 
  tft.setCursor(10, 215); 
  tft.printf("F: %1.2fV  R: %1.2fV  ", g_vFwd, g_vRef);
}
