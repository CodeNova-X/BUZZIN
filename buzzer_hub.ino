// Buzzer HUB - XIAO ESP32-C5. ESP-NOW to controllers + web UI. Button D0 = unlock.
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <esp_now.h>
#include <esp_sleep.h>

const char* WIFI_SSID = "YOUR_WIFI";   // must be a 2.4 GHz network
const char* WIFI_PASS = "YOUR_PASSWORD";

#define BTN D0
#define LEDP D4
#define BUZ D5
#define MAXP 16

enum : uint8_t { T_HELLO = 1, T_BUZZ, T_CONFIG, T_STATE, T_TEST };
struct __attribute__((packed)) Msg { uint8_t type, r, g, b, sound, locked, winner[6]; };


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

// ---- sounds ----
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
int pendingSound = -1;
void playSound(uint8_t s) { pendingSound = s; }
void tickSound() {
  if (pendingSound >= 0) { cur = SND[pendingSound % 8]; ci = 0; nextAt = 0; pendingSound = -1; }
  if (!cur || millis() < nextAt) return;
  if (cur[ci].ms == 0) { noTone(BUZ); cur = nullptr; return; }
  if (cur[ci].f) tone(BUZ, cur[ci].f); else noTone(BUZ);
  nextAt = millis() + cur[ci].ms; ci++;
}

// ---- players ----
struct Player { uint8_t mac[6]; uint8_t r, g, b, sound; char name[16]; int16_t score; };
Player P[MAXP]; uint32_t seen[MAXP]; int nP = 0;
bool locked = false; int winner = -1;  // winner -1 + locked = "lock all"
bool apMode = false;
Preferences prefs; WebServer server(80);
const uint8_t PAL[12][3] = {{255,0,0},{0,120,255},{0,220,60},{255,200,0},{190,0,255},{0,220,220},
                            {255,90,0},{255,0,140},{130,255,0},{255,255,255},{0,255,150},{120,120,255}};
// Random color + sound for a new player, avoiding ones already in use while possible (no web UI needed)
void assignLook(int i) {
  uint16_t usedC = 0, usedS = 0;
  for (int k = 0; k < nP; k++) {
    if (k == i) continue;
    for (int c = 0; c < 12; c++) if (P[k].r == PAL[c][0] && P[k].g == PAL[c][1] && P[k].b == PAL[c][2]) usedC |= 1 << c;
    usedS |= 1 << (P[k].sound & 7);
  }
  if (usedC == 0x0FFF) usedC = 0;
  if (usedS == 0x00FF) usedS = 0;
  int c, s;
  do { c = esp_random() % 12; } while (usedC & (1 << c));
  do { s = esp_random() % 8; } while (usedS & (1 << s));
  P[i].r = PAL[c][0]; P[i].g = PAL[c][1]; P[i].b = PAL[c][2]; P[i].sound = s;
}

void save() { prefs.putUChar("n", nP); prefs.putBytes("p", P, sizeof(Player) * nP); }
int findP(const uint8_t* mac) { for (int i = 0; i < nP; i++) if (!memcmp(P[i].mac, mac, 6)) return i; return -1; }
void addPeer(const uint8_t* mac) {
  if (esp_now_is_peer_exist(mac)) return;
  esp_now_peer_info_t pi = {};
  memcpy(pi.peer_addr, mac, 6); pi.channel = 0; pi.ifidx = apMode ? WIFI_IF_AP : WIFI_IF_STA;
  esp_now_add_peer(&pi);
}
void sendMsg(int i, uint8_t type) {
  Msg m = {}; m.type = type; m.r = P[i].r; m.g = P[i].g; m.b = P[i].b; m.sound = P[i].sound; m.locked = locked;
  if (locked && winner >= 0) memcpy(m.winner, P[winner].mac, 6);
  esp_now_send(P[i].mac, (uint8_t*)&m, sizeof(m));
}
void broadcastState() { for (int i = 0; i < nP; i++) sendMsg(i, T_STATE); }

void showHub() {
  if (locked && winner >= 0) neopixelWrite(LEDP, P[winner].r, P[winner].g, P[winner].b);
  else if (locked) neopixelWrite(LEDP, 80, 0, 0);
  else neopixelWrite(LEDP, 0, 25, 0);
}
void doLock(int w) {
  locked = true; winner = w;
  if (w >= 0) playSound(P[w].sound);
  broadcastState(); showHub();
}
void unlock() { locked = false; winner = -1; broadcastState(); showHub(); }

// ---- ESP-NOW receive queue ----
struct Rx { uint8_t mac[6]; Msg m; };
Rx q[16]; volatile uint8_t qh = 0, qt = 0;
void onRecv(const esp_now_recv_info_t* info, const uint8_t* d, int len) {
  if (len != sizeof(Msg)) return;
  uint8_t n = (qh + 1) % 16; if (n == qt) return;
  memcpy(q[qh].mac, info->src_addr, 6); memcpy(&q[qh].m, d, sizeof(Msg)); qh = n;
}
void processRx() {
  while (qt != qh) {
    Rx r = q[qt]; qt = (qt + 1) % 16;
    int i = findP(r.mac);
    if (r.m.type == T_HELLO) {
      if (i < 0) {
        if (nP >= MAXP) continue;
        i = nP++;
        memset(&P[i], 0, sizeof(Player)); memcpy(P[i].mac, r.mac, 6);
        assignLook(i); snprintf(P[i].name, 16, "Player %d", i + 1);
        save();
      }
      addPeer(r.mac); seen[i] = millis(); sendMsg(i, T_CONFIG);
    } else if (r.m.type == T_BUZZ && i >= 0 && !locked) {
      lastAct = millis();
      doLock(i);
    }
  }
}

// ---- web ----
String stateJson() {
  IPAddress ip = apMode ? WiFi.softAPIP() : WiFi.localIP();
  String s = "{\"l\":" + String(locked ? 1 : 0) + ",\"w\":" + String(winner) + ",\"ip\":\"" + ip.toString() + "\",\"p\":[";
  for (int i = 0; i < nP; i++) {
    char c[8]; snprintf(c, 8, "#%02x%02x%02x", P[i].r, P[i].g, P[i].b);
    if (i) s += ",";
    s += "{\"n\":\"" + String(P[i].name) + "\",\"c\":\"" + c + "\",\"s\":" + P[i].sound + ",\"sc\":" + P[i].score +
         ",\"on\":" + ((seen[i] && millis() - seen[i] < 6000) ? 1 : 0) + "}";
  }
  return s + "]}";
}
void sendState() { server.send(200, "application/json", stateJson()); }
int argI() { int i = server.arg("i").toInt(); return (i >= 0 && i < nP) ? i : -1; }

const char PAGE[] PROGMEM = R"HTML(<!doctype html><html><head><meta name=viewport content="width=device-width,initial-scale=1"><title>Buzzer Hub</title>
<style>
:root{--bg:#0e1014;--card:#171a21;--tx:#e8eaf0;--mut:#8b93a7}
*{box-sizing:border-box}
body{margin:0;font:16px system-ui,sans-serif;background:var(--bg);color:var(--tx)}
header{display:flex;align-items:center;gap:12px;padding:16px 20px;border-bottom:1px solid #232733}
h1{font-size:18px;margin:0;flex:1}
#dot{width:10px;height:10px;border-radius:50%;background:#e5484d}#dot.on{background:#30a46c}
main{max-width:1000px;margin:auto;padding:20px}
#banner{padding:28px;border-radius:16px;text-align:center;background:var(--card);border:2px solid #232733;transition:.2s}
#bt{font-size:30px;font-weight:700}#bs{font-size:14px;color:var(--mut);margin-top:6px}
.bar{display:flex;gap:10px;margin:16px 0}
button{font:inherit;border:0;border-radius:10px;padding:12px 18px;background:#262b38;color:var(--tx);cursor:pointer}
button:hover{background:#30374a}.pri{background:#3e63dd;flex:1}.pri:hover{background:#5472e4}.dng{background:#4a2327;padding:8px 12px}
#grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(280px,1fr));gap:14px}
.card{background:var(--card);border-radius:14px;padding:16px;border-top:5px solid var(--c);opacity:.5;transition:.2s}
.card.on{opacity:1}.card.win{box-shadow:0 0 0 2px var(--c),0 0 28px var(--c)}
.row{display:flex;gap:8px;align-items:center;margin-top:10px}
input,select{font:inherit;background:#0e1014;color:var(--tx);border:1px solid #2b3040;border-radius:8px;padding:8px;min-width:0}
input[type=text]{flex:1;font-weight:600}input[type=color]{width:46px;height:40px;padding:2px}select{flex:1}
.score{font-size:30px;font-weight:700;flex:1;text-align:center}
.st{font-size:13px;color:var(--mut);flex:1}
#empty{color:var(--mut);text-align:center;padding:40px}
</style></head><body>
<header><div id=dot></div><h1>🔔 Buzzer Hub</h1><span class=st id=ip style="flex:none"></span></header>
<main>
<div id=banner><div id=bt></div><div id=bs></div></div>
<div class=bar><button class=pri onclick="api('reset')">Reset / Unlock</button><button onclick="api('lock')">Lock all</button></div>
<div id=grid></div><div id=empty>Turn on a controller and it will appear here.</div>
</main>
<script>
const SOUNDS=["Beep","Ding-Dong","Rise","Fall","Siren","Buzz","Chirp","Fanfare"];
const $=id=>document.getElementById(id);let n=-1;
const api=p=>fetch('/api/'+p).then(r=>r.json()).then(render);
function poll(){api('state').then(()=>$('dot').className='on').catch(()=>$('dot').className='').finally(()=>setTimeout(poll,400))}
function build(d){
 $('grid').innerHTML=d.p.map((p,i)=>`<div class=card id=c${i}>
 <div class=row style="margin:0"><input type=text id=n${i} maxlength=15><input type=color id=k${i}></div>
 <div class=row><select id=s${i}>${SOUNDS.map((s,j)=>`<option value=${j}>${s}</option>`).join('')}</select><button onclick="api('test?i=${i}')">▶ Test</button></div>
 <div class=row><button onclick="api('score?i=${i}&d=-1')">−</button><div class=score id=sc${i}></div><button onclick="api('score?i=${i}&d=1')">+</button></div>
 <div class=row><span class=st id=st${i}></span><button class=dng onclick="if(confirm('Remove?'))api('forget?i=${i}')">✕</button></div></div>`).join('');
 d.p.forEach((p,i)=>{$('n'+i).value=p.n;$('k'+i).value=p.c;$('s'+i).value=p.s;['n','k','s'].forEach(x=>$(x+i).onchange=()=>save(i))});
}
function save(i){api(`cfg?i=${i}&c=${$('k'+i).value.slice(1)}&s=${$('s'+i).value}&n=${encodeURIComponent($('n'+i).value)}`)}
function render(d){
 if(d.p.length!==n){n=d.p.length;build(d)}
 $('empty').style.display=n?'none':'block';$('ip').textContent=d.ip;
 const w=d.p[d.w],b=$('banner');
 if(d.l&&w){b.style.borderColor=w.c;b.style.background=w.c+'33';$('bt').textContent=w.n+' buzzed first!';$('bs').textContent='Locked — press Reset or the hub button'}
 else if(d.l){b.style.borderColor='#e5484d';b.style.background='#e5484d22';$('bt').textContent='Locked';$('bs').textContent='All buzzers disabled'}
 else{b.style.borderColor='#30a46c';b.style.background='#30a46c18';$('bt').textContent='Ready';$('bs').textContent='Waiting for a buzz…'}
 d.p.forEach((p,i)=>{const c=$('c'+i);c.style.setProperty('--c',p.c);c.classList.toggle('on',!!p.on);c.classList.toggle('win',d.l&&d.w==i);
  $('sc'+i).textContent=p.sc;$('st'+i).textContent=p.on?(d.l&&d.w==i?'Winner':'Online'):'Offline'});
}
poll();
</script></body></html>)HTML";

void setup() {
  Serial.begin(115200);
  pinMode(BTN, INPUT_PULLUP);
  lastAct = millis();
  neopixelWrite(LEDP, 20, 10, 0);
  prefs.begin("buz", false);
  nP = prefs.getUChar("n", 0);
  if (nP > MAXP || prefs.getBytesLength("p") != sizeof(Player) * nP) nP = 0;
  if (nP) prefs.getBytes("p", P, sizeof(Player) * nP);

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  uint32_t t = millis();
  if (strlen(WIFI_SSID) && strcmp(WIFI_SSID, "YOUR_WIFI")) {
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED && millis() - t < 10000) delay(200);
  }
  if (WiFi.status() != WL_CONNECTED) {  // fallback: own access point on channel 1
    apMode = true;
    WiFi.mode(WIFI_AP);
    WiFi.softAP("BuzzerHub", "buzzer123", 1);
  }
  Serial.println(apMode ? WiFi.softAPIP() : WiFi.localIP());

  esp_now_init();
  esp_now_register_recv_cb(onRecv);
  for (int i = 0; i < nP; i++) addPeer(P[i].mac);

  MDNS.begin("buzzer");  // http://buzzer.local
  server.on("/", []() { server.send_P(200, "text/html", PAGE); });
  server.on("/api/state", sendState);
  server.on("/api/reset", []() { lastAct = millis(); unlock(); sendState(); });
  server.on("/api/lock", []() { lastAct = millis(); doLock(-1); sendState(); });
  server.on("/api/test", []() { lastAct = millis(); int i = argI(); if (i >= 0) sendMsg(i, T_TEST); sendState(); });
  server.on("/api/score", []() {
    lastAct = millis();
    int i = argI(); if (i >= 0) { P[i].score += server.arg("d").toInt(); save(); }
    sendState();
  });
  server.on("/api/cfg", []() {
    lastAct = millis();
    int i = argI();
    if (i >= 0) {
      uint32_t c = strtoul(server.arg("c").c_str(), nullptr, 16);
      P[i].r = c >> 16; P[i].g = c >> 8; P[i].b = c;
      P[i].sound = constrain(server.arg("s").toInt(), 0, 7);
      String n = server.arg("n"); n.trim();
      memset(P[i].name, 0, 16);
      for (int k = 0; k < 15 && k < (int)n.length(); k++) {
        char ch = n[k]; P[i].name[k] = (ch < 32 || strchr("\"\\<>&", ch)) ? '_' : ch;
      }
      save(); sendMsg(i, T_CONFIG); showHub();
    }
    sendState();
  });
  server.on("/api/forget", []() {
    lastAct = millis();
    int i = argI();
    if (i >= 0) {
      esp_now_del_peer(P[i].mac);
      for (int k = i; k < nP - 1; k++) { P[k] = P[k + 1]; seen[k] = seen[k + 1]; }
      nP--;
      if (winner == i) { locked = false; winner = -1; } else if (winner > i) winner--;
      save(); broadcastState(); showHub();
    }
    sendState();
  });
  server.begin();
  showHub();
}

bool wasDown = false; uint32_t lastPress = 0;
void loop() {
  server.handleClient();
  processRx();
  tickSound();
  checkSleep();
  bool down = digitalRead(BTN) == LOW;
  if (down && !wasDown && millis() - lastPress > 250) { lastPress = millis(); unlock(); }
  wasDown = down;
}
