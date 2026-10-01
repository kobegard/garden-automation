// WH52-Rival bench test: confirm review findings F3 (moisture receive path),
// the R8/IO10 loading effect, F4 (TH1 on ADC2) and the battery-sense divider,
// on the CURRENT board (netlist PCB1 2026-09-09) before committing to a respin.
//
// Board: "ESP32C3 Dev Module", Arduino-ESP32 core 3.x.
// REQUIRED: Tools > USB CDC On Boot > Enabled (the WH52 has no UART pins out).
// Open the Serial Monitor at 115200 and move the tip between: air, tap water,
// moist soil, dry soil. Each report takes ~4 s. See README.md for what to expect.
//
// Pin map (netlist PCB1 2026-09-09, ESP32-C3-MINI-1 module pins):
//   IO4  (pin 18) -> C4 -> R6 -> P1   moisture excitation
//   IO1  (pin 13) <- P1 node          moisture "loading" fallback (ADC1_CH1)
//   IO3  (pin 6)  <- D2/C7/R10 <- P2  moisture receive (ADC1_CH3)  [F3]
//   IO10 (pin 16) -> C6 -> P3         EC excitation
//   IO6  (pin 20) <- D1/C8 <- P4      EC receive, digital only (R8 1k to GND)
//   IO5  (pin 19) <- R11/TH1 divider  soil temperature (ADC2_CH0)  [F4]
//   IO0  (pin 12) <- Vbat/2 divider   battery sense (ADC1_CH0)
//   IO2  (pin 5)  -> divider bottom   drive LOW only while reading the battery

#if !ARDUINO_USB_CDC_ON_BOOT
#error "Enable Tools > USB CDC On Boot, or Serial goes to pins that aren't wired out"
#endif

#include <Arduino.h>
#include <math.h>

static const int PIN_MOIST_TX = 4;
static const int PIN_MOIST_MON = 1;
static const int PIN_MOIST_RX = 3;
static const int PIN_EC_TX = 10;
static const int PIN_EC_RX = 6;
static const int PIN_TH1 = 5;
static const int PIN_BATT = 0;
static const int PIN_BATT_EN = 2;

static const uint32_t FREQS[] = {200, 1000};  // Hz; research doc: same shape 1-10 kHz
static const float VCC_MV = 3300.0f;          // RT9080-33 output
static const float R11 = 10000.0f, NTC_R25 = 10000.0f, NTC_B = 3950.0f;

static void hiZ(int pin) { pinMode(pin, INPUT); }

static float avgMv(int pin, int n) {
  uint32_t sum = 0;
  for (int i = 0; i < n; i++) sum += analogReadMilliVolts(pin);
  return float(sum) / n;
}

// Battery: drive IO2 low, read Vbat/2 on IO0, release IO2.
static float readBatteryV() {
  pinMode(PIN_BATT_EN, OUTPUT);
  digitalWrite(PIN_BATT_EN, LOW);
  delay(5);  // C10 10 nF through ~50 k settles in well under 1 ms
  float mv = avgMv(PIN_BATT, 32);
  hiZ(PIN_BATT_EN);
  return 2.0f * mv / 1000.0f;
}

struct MoistResult {
  float io3_mv;    // as-built receive path; expect ~0 if F3 is real
  float io1_pp;    // free-running peak-to-peak on the P1 node
  float io1_hi;    // P1 node shortly after each rising edge
  float io1_lo;    // P1 node shortly after each falling edge (clips at 0)
};

static MoistResult moisture(uint32_t freq, bool ec_tx_low) {
  MoistResult r{};
  // Research doc: IO10 driven low gives the excitation a path to GND through
  // C6/P3; hi-Z removes it. Measure both so the effect shows up.
  if (ec_tx_low) {
    pinMode(PIN_EC_TX, OUTPUT);
    digitalWrite(PIN_EC_TX, LOW);
  } else {
    hiZ(PIN_EC_TX);
  }

  // 1) Free-running square wave: IO3 DC level and IO1 min/max.
  ledcAttach(PIN_MOIST_TX, freq, 8);
  ledcWrite(PIN_MOIST_TX, 128);
  delay(300);  // C7 (100 nF) x R10 (100 k) = 10 ms; give it plenty
  r.io3_mv = avgMv(PIN_MOIST_RX, 64);
  uint16_t mn = 0xFFFF, mx = 0;
  for (int i = 0; i < 3000; i++) {
    uint16_t v = analogReadMilliVolts(PIN_MOIST_MON);
    mn = min(mn, v);
    mx = max(mx, v);
  }
  r.io1_pp = mx - mn;
  ledcDetach(PIN_MOIST_TX);

  // 2) Edge-synchronous: sample the P1 node a fixed delay after each edge.
  pinMode(PIN_MOIST_TX, OUTPUT);
  uint32_t half_us = 500000UL / freq;
  uint32_t settle_us = half_us / 4;
  uint32_t hi = 0, lo = 0;
  const int cycles = 64;
  for (int i = 0; i < cycles + 16; i++) {
    digitalWrite(PIN_MOIST_TX, HIGH);
    delayMicroseconds(settle_us);
    uint16_t h = analogReadMilliVolts(PIN_MOIST_MON);
    delayMicroseconds(half_us - settle_us);
    digitalWrite(PIN_MOIST_TX, LOW);
    delayMicroseconds(settle_us);
    uint16_t l = analogReadMilliVolts(PIN_MOIST_MON);
    delayMicroseconds(half_us - settle_us);
    if (i >= 16) {  // skip start-up cycles while C4 charges
      hi += h;
      lo += l;
    }
  }
  r.io1_hi = float(hi) / cycles;
  r.io1_lo = float(lo) / cycles;
  hiZ(PIN_MOIST_TX);
  hiZ(PIN_EC_TX);
  return r;
}

// EC: excite P3, count how often the peak-held IO6 node reads HIGH.
static float ecHighPercent(uint32_t freq) {
  hiZ(PIN_MOIST_TX);
  pinMode(PIN_EC_RX, INPUT);
  ledcAttach(PIN_EC_TX, freq, 8);
  ledcWrite(PIN_EC_TX, 128);
  delay(50);
  uint32_t high = 0;
  const uint32_t n = 20000;
  for (uint32_t i = 0; i < n; i++) high += digitalRead(PIN_EC_RX);
  ledcDetach(PIN_EC_TX);
  hiZ(PIN_EC_TX);
  return 100.0f * high / n;
}

// TH1 on IO5 = ADC2. On the C3, ESP-IDF 5 refuses ADC2 oneshot by default,
// so this may read a flat 0 (that alone confirms F4). Spread = instability.
static void thermistor(float *mv, float *spread, float *temp_c) {
  uint16_t mn = 0xFFFF, mx = 0;
  uint32_t sum = 0;
  const int n = 64;
  for (int i = 0; i < n; i++) {
    uint16_t v = analogReadMilliVolts(PIN_TH1);
    sum += v;
    mn = min(mn, v);
    mx = max(mx, v);
  }
  *mv = float(sum) / n;
  *spread = mx - mn;
  *temp_c = NAN;
  if (*mv > 1 && *mv < VCC_MV - 1) {
    float r_ntc = R11 * *mv / (VCC_MV - *mv);
    *temp_c = 1.0f / (1.0f / 298.15f + logf(r_ntc / NTC_R25) / NTC_B) - 273.15f;
  }
}

void setup() {
  Serial.begin(115200);
  uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 5000) delay(10);
  analogSetAttenuation(ADC_11db);  // ~0-2.5 V usable on the C3
  for (int p : {PIN_MOIST_TX, PIN_EC_TX, PIN_BATT_EN}) hiZ(p);
  Serial.println();
  Serial.println("WH52 bench test. No WiFi is used, so ADC2 has no radio contention.");
  Serial.println("Move the tip: air -> tap water -> moist soil -> dry soil.");
}

void loop() {
  float vbat = readBatteryV();
  Serial.printf("\n--- %lus  battery %.2f V%s\n", millis() / 1000, vbat,
                vbat < 3.6f ? "  (LOW: below ~3.6 V the LDO drops out on TX)" : "");

  for (uint32_t f : FREQS) {
    MoistResult a = moisture(f, true);   // as built: IO10 low
    MoistResult b = moisture(f, false);  // IO10 hi-Z
    Serial.printf("moisture %4lu Hz | IO3 %6.0f mV | IO1 p-p %4.0f  hi %4.0f lo %4.0f mV"
                  " | IO10 hi-Z: IO3 %6.0f  IO1 p-p %4.0f  hi %4.0f mV\n",
                  f, a.io3_mv, a.io1_pp, a.io1_hi, a.io1_lo, b.io3_mv, b.io1_pp, b.io1_hi);
  }
  for (uint32_t f : FREQS)
    Serial.printf("EC       %4lu Hz | IO6 high %5.1f %% of samples\n", f, ecHighPercent(f));

  float mv, spread, tc;
  thermistor(&mv, &spread, &tc);
  Serial.printf("TH1 (ADC2) | %6.0f mV  spread %4.0f mV  -> %s", mv, spread, isnan(tc) ? "no reading" : "");
  if (!isnan(tc)) Serial.printf("%.1f C", tc);
  Serial.println(mv < 1 ? "  (flat 0: ADC2 unavailable -> F4 confirmed)" : "");

  delay(1000);
}
