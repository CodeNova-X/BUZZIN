// Buzzer CONTROLLER - XIAO ESP32-C5 (ESP-NOW). Button D0 (to GND), WS2812 on D4, passive buzzer on D5.
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <esp_sleep.h>

#define BTN D0
#define LEDP D4
#define BUZ D5

enum : uint8_t { T_HELLO = 1, T_BUZZ, T_CONFIG, T_STATE, T_TEST };
struct __attribute__((packed)) Msg { uint8_t type, r, g, b, sound, locked, winner[6]; };

// ---- sounds (freq Hz, ms); {0,0} ends a sound ----
struct Note { uint16_t f, ms; };
const Note S0[] = {{880,250},{0,0}};
const Note S1[] = {{988,150},{740,250},{0,0}};
const Note S2[] = {{440,80},{660,80},{880,120},{0,0}};
const Note S3[] = {{880,80},{660,80},{440,150},{0,0}};
const Note S4[] = {{800,120},{1000,120},{800,120},{1000,120},{0,0}};
const Note S5[] = {{150,350},{0,0}};
const Note S6[] = {{2000,50},{0,40},{2000,50},{0,40},{2400,80},{0,0}};
const Note S7[] = {{523,100},{659,100},{784,100},{1047,250},{0,0}};
const Note* const SND[] = {S0,S1,S2,S3,S4,S5,S6,S7};
const Note* cur = nullptr; uint8_t ci; uint32_t nextAt;
volatile int pendingSound = -1;
void playSound(uint8_t s) { pendingSound = s; }
void tickSound() {
  if (pendingSound >= 0) { cur = SND[pendingSound % 8]; ci = 0; nextAt = 0; pendingSound = -1; }
  if (!cur || millis() < nextAt) return;
  if (cur[ci].ms == 0) { noTone(BUZ); cur = nullptr; return; }
  if (cur[ci].f) tone(BUZ, cur[ci].f); else noTone(BUZ);
  nextAt = millis() + cur[ci].ms; ci++;
}


// ---- deep sleep after 1.5 h without activity; button wakes the board ----
#define SLEEP_MS (90UL * 60UL * 1000UL)
volatile uint32_t lastAct = 0;
void goSleep() {
  noTone(BUZ);
  neopixelWrite(LEDP, 0, 0, 0);
  while (digitalRead(BTN) == LOW) delay(10);  // must be released or it wakes instantly
  delay(50);
  esp_deep_sleep_enable_gpio_wakeup(1ULL << digitalPinToGPIONumber(BTN), ESP_GPIO_WAKEUP_GPIO_LOW);
  esp_deep_sleep_start();
}
void checkSleep() {
  if (digitalRead(BTN) == LOW) lastAct = millis();
  if (millis() - lastAct > SLEEP_MS) goSleep();
}

// ---- state ----
uint8_t myMac[6], hubMac[6];
volatile bool hubKnown = false;
uint8_t cr = 0, cg = 120, cb = 255, cs = 0;
volatile uint8_t st = 0;  // 0 idle, 1 locked out, 2 winner
uint32_t lastRx, lastHB, lastPress, lastLed = 0xFFFFFFFF;
bool wasDown = false;
const uint8_t BC[6] = {255,255,255,255,255,255};

void sendMsg(uint8_t type, const uint8_t* dest) {
  Msg m = {}; m.type = type;
  esp_now_send(dest, (uint8_t*)&m, sizeof(m));
}

void onRecv(const esp_now_recv_info_t* info, const uint8_t* d, int len) {
  if (len != sizeof(Msg)) return;
  Msg m; memcpy(&m, d, sizeof(m));
  if (hubKnown && memcmp(info->src_addr, hubMac, 6)) return;
  if (!hubKnown) {
    if (m.type != T_CONFIG) return;
    memcpy(hubMac, info->src_addr, 6); hubKnown = true;
  }
  lastRx = millis();
  if (m.type == T_TEST) { playSound(m.sound); return; }
  if (m.type == T_CONFIG) { cr = m.r; cg = m.g; cb = m.b; cs = m.sound; }
  uint8_t ns = !m.locked ? 0 : (!memcmp(m.winner, myMac, 6) ? 2 : 1);
  if (ns == 2 && st != 2) playSound(cs);
  if (ns != st) lastAct = millis();  // game activity keeps it awake
  st = ns;
}

void setLed(uint8_t r, uint8_t g, uint8_t b, float k) {
  uint32_t key = ((uint32_t)(r * k) << 16) | ((uint32_t)(g * k) << 8) | (uint32_t)(b * k);
  if (key == lastLed) return;
  lastLed = key;
  neopixelWrite(LEDP, r * k, g * k, b * k);
}

void findHub() {
  hubKnown = false;
  setLed(255, 120, 0, 0.1);  // dim orange = searching
  while (true) {
    for (int ch = 1; ch <= 13; ch++) {
      esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
      sendMsg(T_HELLO, BC);
      uint32_t t = millis();
      while (millis() - t < 120) {
        tickSound();
        checkSleep();
        if (hubKnown) {
          esp_now_peer_info_t pi = {};
          memcpy(pi.peer_addr, hubMac, 6); pi.channel = 0; pi.ifidx = WIFI_IF_STA;
          esp_now_add_peer(&pi);
          lastRx = lastHB = millis();
          return;
        }
        delay(1);
      }
    }
  }
}

void setup() {
  pinMode(BTN, INPUT_PULLUP);
  lastAct = millis();
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  WiFi.setSleep(false);
  WiFi.macAddress(myMac);
  esp_now_init();
  esp_now_register_recv_cb(onRecv);
  esp_now_peer_info_t pi = {};
  memcpy(pi.peer_addr, BC, 6); pi.channel = 0; pi.ifidx = WIFI_IF_STA;
  esp_now_add_peer(&pi);
  findHub();
}

void loop() {
  tickSound();
  checkSleep();

  if (millis() - lastHB > 2000) { lastHB = millis(); sendMsg(T_HELLO, hubMac); }
  if (millis() - lastRx > 7000) { esp_now_del_peer(hubMac); st = 0; findHub(); }

  bool down = digitalRead(BTN) == LOW;
  if (down && !wasDown && millis() - lastPress > 250) {
    lastPress = millis();
    if (st == 0) sendMsg(T_BUZZ, hubMac);
  }
  wasDown = down;

  // idle = medium, locked out = a bit dimmer, winner = full
  setLed(cr, cg, cb, st == 2 ? 1.0 : st == 1 ? 0.12 : 0.40);
}
