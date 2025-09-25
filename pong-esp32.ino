#include <WiFi.h>
#include <WebServer.h>
#include <U8g2lib.h>
#include <Wire.h>

// ================= OLED (Page Buffer) =================
U8G2_SH1106_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// ================= Joysticks =================
#define JOY1_Y 34
#define JOY1_SW 25
#define JOY2_Y 35
#define JOY2_SW 26

// ================= Game State =================
int p1Y = 30, p2Y = 30;
int ballX = 64, ballY = 32;
int ballDX = 1, ballDY = 1;
const int paddleH = 16, paddleW = 2;
int p1Score = 0, p2Score = 0;
int gameMode = 0;             // 0=PvP, 1=PvAI
bool selectConfirmed = false;

// ================= Wi-Fi / WebServer =================
WebServer server(80);
static const char *SSID = "ESP32-PONG";
static const char *PASS = "12345678";

// HTML + CSS + JS (เก็บใน Flash)
static const char INDEX_HTML[] PROGMEM = R"HTML(
<!DOCTYPE html><html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP32 Pong Scoreboard</title>
<style>
  :root { --bg:#0b0f14; --panel:#111826; --accent:#00d0ff; --muted:#8aa0b3; --good:#38d39f; --warn:#ffb020; }
  * { box-sizing:border-box; font-family: ui-monospace, SFMono-Regular, Menlo, Consolas, "Liberation Mono", monospace; }
  body { margin:0; background:radial-gradient(1200px 600px at 50% -10%, #142233 0%, #0b0f14 60%, #06080c 100%); color:#e6edf3; }
  .wrap { max-width: 680px; margin: 32px auto; padding: 16px; }
  .card { background: linear-gradient(180deg, rgba(255,255,255,0.02), rgba(255,255,255,0.00));
          border:1px solid #1f2a3a; border-radius:16px; padding:20px; box-shadow:0 10px 30px rgba(0,0,0,.35); }
  .title { display:flex; align-items:center; gap:12px; margin-bottom:12px; }
  .dot { width:10px; height:10px; border-radius:50%; background:var(--accent); box-shadow:0 0 12px var(--accent); }
  h1 { font-size:20px; margin:0; letter-spacing:.5px; }
  .ip { color:var(--muted); font-size:12px; }
  .score { display:grid; grid-template-columns:1fr auto 1fr; align-items:center; gap:10px; margin:10px 0 6px; }
  .side { background: rgba(0,208,255,.06); border:1px solid rgba(0,208,255,.25); border-radius:12px; padding:12px;
          text-align:center; }
  .side h2 { margin:0 0 2px; font-weight:600; font-size:13px; color:#b7c7d9; }
  .val { font-size:34px; font-weight:800; text-shadow:0 0 10px rgba(0,208,255,.25); }
  .dash { color:#3d556b; font-size:18px; }
  .meta { display:flex; justify-content:space-between; align-items:center; margin-top:8px; color:var(--muted); font-size:12px; }
  .pill { display:inline-flex; align-items:center; gap:6px; padding:6px 10px; border-radius:999px;
          border:1px solid #1f2a3a; background:#0d1420; }
  .led { width:8px; height:8px; border-radius:50%; background:var(--good); box-shadow:0 0 10px var(--good); }
  button { border:1px solid #1f2a3a; background:#0d1420; color:#dce6f3; padding:8px 12px; border-radius:10px; cursor:pointer; }
  button:hover { border-color:#2c3c52; }
  .grid { display:grid; grid-template-columns:1fr 1fr; gap:10px; margin-top:10px; }
  .kv { border:1px dashed #203043; border-radius:10px; padding:8px; color:#b8c7d6; font-size:12px; text-align:center; }
  .mono { font-variant-numeric: tabular-nums; }
  footer { margin-top:14px; text-align:center; color:#516e88; font-size:11px; }
</style>
</head>
<body>
  <div class="wrap">
    <div class="card">
      <div class="title">
        <div class="dot"></div>
        <div>
          <h1>ESP32 Pong — Scoreboard</h1>
          <div class="ip">AP: <span class="mono">192.168.4.1</span> · SSID: <span class="mono">ESP32-PONG</span></div>
        </div>
      </div>

      <div class="score">
        <div class="side">
          <h2 id="lName">P1</h2>
          <div class="val mono" id="p1">0</div>
        </div>
        <div class="dash">vs</div>
        <div class="side">
          <h2 id="rName">P2</h2>
          <div class="val mono" id="p2">0</div>
        </div>
      </div>

      <div class="meta">
        <div class="pill"><span class="led" id="led"></span><span id="mode">Mode: PvP</span></div>
        <button id="reset">Reset Score</button>
      </div>

      <div class="grid">
        <div class="kv">Ball <span class="mono" id="ball">(64,32)</span></div>
        <div class="kv">FPS <span class="mono" id="fps">~33</span></div>
      </div>

      <footer>Auto-refresh via /score.json every 500ms</footer>
    </div>
  </div>

<script>
const $ = (id)=>document.getElementById(id);

async function tick(){
  try{
    const r = await fetch('/score.json',{cache:'no-store'});
    if(!r.ok) throw new Error(r.status);
    const d = await r.json();
    $('p1').textContent = d.P1;
    $('p2').textContent = d.P2;
    $('mode').textContent = 'Mode: ' + (d.mode===0 ? 'PvP' : 'PvAI');
    $('lName').textContent = d.mode===0 ? 'P1' : 'YOU';
    $('rName').textContent = d.mode===0 ? 'P2' : 'AI';
    $('ball').textContent = '('+d.ballX+','+d.ballY+')';
    $('led').style.background = '#38d39f';
    $('led').style.boxShadow = '0 0 10px #38d39f';
  }catch(e){
    $('led').style.background = '#ff5d5d';
    $('led').style.boxShadow = '0 0 10px #ff5d5d';
  }
}
setInterval(tick, 500);

document.getElementById('reset').addEventListener('click', async ()=>{
  try{ await fetch('/reset', {method:'POST'}); }catch(e){}
});
tick();
</script>
</body></html>
)HTML";

// ================= Handlers =================
void handleRoot() {
  server.send_P(200, "text/html", INDEX_HTML);
}

void handleScoreJson() {
  char json[96];
  // {"P1":0,"P2":0,"mode":0,"ballX":64,"ballY":32}
  snprintf(json, sizeof(json),
           "{\"P1\":%d,\"P2\":%d,\"mode\":%d,\"ballX\":%d,\"ballY\":%d}",
           p1Score, p2Score, gameMode, ballX, ballY);
  server.send(200, "application/json", json);
}

void handleReset() {
  p1Score = p2Score = 0;
  server.send(200, "text/plain", "OK");
}

// ================= Core =================
void drawMenu() {
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_6x12_tr);
    u8g2.drawStr(10, 14, "Select Game Mode");
    if (gameMode == 0) {
      u8g2.drawStr(16, 36, "> Player vs Player");
      u8g2.drawStr(16, 52, "  Player vs AI");
    } else {
      u8g2.drawStr(16, 36, "  Player vs Player");
      u8g2.drawStr(16, 52, "> Player vs AI");
    }
  } while (u8g2.nextPage());
}

void readJoystickP1() {
  int y = analogRead(JOY1_Y);
  if (y < 1500 && p1Y > 0) p1Y -= 2;
  else if (y > 3000 && p1Y < 64 - paddleH) p1Y += 2;
}

void readJoystickP2() {
  int y = analogRead(JOY2_Y);
  if (y < 1500 && p2Y > 0) p2Y -= 2;
  else if (y > 3000 && p2Y < 64 - paddleH) p2Y += 2;
}

void updateBall() {
  ballX += ballDX; ballY += ballDY;

  if (ballY <= 0 || ballY >= 63) ballDY = -ballDY;

  if (ballX <= paddleW &&
      ballY >= p1Y && ballY <= p1Y + paddleH) {
    ballDX = -ballDX;
    // เพิ่มมุมตามตำแหน่งชน (spin เบาๆ)
    ballDY += (ballY - (p1Y + paddleH/2)) / 4;
  }

  if (ballX >= 127 - paddleW &&
      ballY >= p2Y && ballY <= p2Y + paddleH) {
    ballDX = -ballDX;
    ballDY += (ballY - (p2Y + paddleH/2)) / 4;
  }

  if (ballX <= 0) { p2Score++; resetBall(); }
  if (ballX >= 127) { p1Score++; resetBall(); }
}

void resetBall() {
  ballX = 64; ballY = 32;
  ballDX = (random(0,2) == 0) ? -1 : 1;
  ballDY = random(-1,2);
}

void drawGame(const char* left, const char* right) {
  char bufL[12], bufR[12];
  snprintf(bufL, sizeof(bufL), "%s:%d", left, p1Score);
  snprintf(bufR, sizeof(bufR), "%s:%d", right, p2Score);

  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_6x12_tr);
    u8g2.drawBox(0, p1Y, paddleW, paddleH);
    u8g2.drawBox(127 - paddleW, p2Y, paddleW, paddleH);
    u8g2.drawBox(ballX, ballY, 2, 2);
    u8g2.drawStr(4, 10, bufL);
    u8g2.drawStr(92, 10, bufR);
  } while (u8g2.nextPage());
}

void selectMode() {
  int y1 = analogRead(JOY1_Y);
  if (y1 < 1500) { gameMode = 0; drawMenu(); delay(200); }
  else if (y1 > 3000) { gameMode = 1; drawMenu(); delay(200); }

  if (digitalRead(JOY1_SW) == LOW) {
    selectConfirmed = true;
  }
}

void playPvP() {
  readJoystickP1();
  readJoystickP2();
  updateBall();
  drawGame("P1","P2");
}

void playPvAI() {
  readJoystickP1();
  // AI: ติดตามบอลแบบง่าย
  if (ballY < p2Y + paddleH/2) p2Y -= 2;
  else if (ballY > p2Y + paddleH/2) p2Y += 2;
  updateBall();
  drawGame("YOU","AI");
}

// ================= Arduino =================
void setup() {
  // I2C OLED + GPIO
  u8g2.begin();
  pinMode(JOY1_SW, INPUT_PULLUP);
  pinMode(JOY2_SW, INPUT_PULLUP);

  // Wi-Fi SoftAP
  WiFi.softAP(SSID, PASS);
  // Fixed AP IP 192.168.4.1 (ค่ามาตรฐานของ SoftAP ESP32)

  // Routes
  server.on("/", handleRoot);
  server.on("/score.json", handleScoreJson);
  server.on("/reset", HTTP_POST, handleReset);
  server.begin();

  drawMenu();
}

void loop() {
  server.handleClient();

  if (!selectConfirmed) {
    selectMode();
  } else {
    if (gameMode == 0) playPvP();
    else playPvAI();
  }
}
