#include <WiFi.h>
#include <WebServer.h>
#include <U8g2lib.h>
#include <Wire.h>

// ===== OLED SH1106 (Page Buffer) =====
U8G2_SH1106_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// ===== Joysticks =====
#define JOY1_Y 34
#define JOY2_Y 35

// ===== Buzzer =====
#define BUZZER_PIN 25

// ===== Buttons =====
#define BTN1 13  // PvP
#define BTN2 12  // PvAI
#define BTN3 14  // AIvsAI

// ===== Game State =====
int p1Y=30, p2Y=30;
int ballX=64, ballY=32;
int ballDX=1, ballDY=1;
const int paddleH=16, paddleW=2;
int p1Score=0, p2Score=0;
int gameMode=-1;
bool gameOver=false;
unsigned long gameOverTime=0;

// ===== Winner Enum =====
enum Winner {NONE, W_P1, W_P2, W_YOU, W_AI, W_AI1, W_AI2};
Winner winner=NONE, lastWinner=NONE;

inline const char* winnerName(Winner w){
  switch(w){
    case W_P1: return "P1";
    case W_P2: return "P2";
    case W_YOU: return "YOU";
    case W_AI: return "AI";
    case W_AI1: return "AI1";
    case W_AI2: return "AI2";
    default: return "";
  }
}

// ===== AI Accuracy (use uint8_t) =====
uint8_t ai1Accuracy=0;
uint8_t ai2Accuracy=0;

// ===== OLED Brightness =====
uint8_t oledBrightness = 128;

// ===== WiFi AP/Web =====
WebServer server(80);
static const char* SSID = "ESP32-PONG";
static const char* PASS = "12345678";

// ===== HTML (Flash) =====
static const char INDEX_HTML[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8"/>
<title>ESP32 Pong</title>
<style>
body{font-family:monospace;background:#0b0f14;color:#eee;text-align:center;padding:20px;margin:0}
h1{font-size:28px;color:#38bdf8;text-shadow:0 0 6px #0ff;margin-bottom:16px}
.score{font-size:28px;font-weight:bold;margin:10px 0;color:#0ff;text-shadow:0 0 6px #0ff}
.mode{color:#94a3b8;margin-bottom:4px;font-size:14px}
.aiinfo{color:#38bdf8;font-size:13px;margin-bottom:12px;height:16px;letter-spacing:1px}
.win{font-size:18px;color:#0ff;margin-top:10px;text-shadow:0 0 8px #0ff}
button{margin:6px;padding:10px 16px;font-size:14px;font-weight:bold;border:2px solid #38bdf8;border-radius:6px;
background:#0b1724;color:#38bdf8;cursor:pointer;transition:all .2s;box-shadow:0 0 6px rgba(56,189,248,.5)}
button:hover{background:#38bdf8;color:#0b1724;box-shadow:0 0 12px #38bdf8}
button:active{transform:scale(.95)}
label{font-size:14px;color:#94a3b8}
input[type=range]{width:220px;margin-top:10px}
canvas{background:#000;border:2px solid #38bdf8;margin-top:15px;image-rendering:pixelated}
.row{display:flex;gap:10px;justify-content:center;flex-wrap:wrap;margin-top:8px}
</style>
</head>
<body>
<h1>ESP32 Pong</h1>
<div class="score" id="sc">0 - 0</div>
<div class="mode" id="md">Mode: Not Selected</div>
<div class="aiinfo" id="aiinfo"></div>
<div class="win" id="win"></div>
<div class="row">
  <button onclick="fetch('/mode?m=0')">Player vs Player</button>
  <button onclick="fetch('/mode?m=1')">Player vs AI</button>
  <button onclick="fetch('/mode?m=2').then(()=>setTimeout(poll,150))">AI vs AI</button>
</div>
<div class="row">
  <button onclick="fetch('/reset',{method:'POST'}).then(()=>setTimeout(poll,150))">Reset</button>
  <button onclick="fetch('/ping')">Ping</button>
  <button onclick="location.reload()">Refresh</button>
</div>
<div style="margin-top:8px">
  <label>Brightness: <span id="brVal">128</span></label><br>
  <input type="number" id="br" min="0" max="255" value="128"
         oninput="brVal.textContent=this.value; fetch('/brightness?val='+this.value)">
</div>
<canvas id="game" width="128" height="64"></canvas>
<script>
const W=128,H=64,PW=2,PH=16;
async function poll(){
  try{
    const r = await fetch('/score.json');
    const d = await r.json();
    sc.textContent = d.a + " - " + d.b;
    md.textContent = "Mode: " + (d.m==-1?"Not Selected":(d.m==0?"PvP":(d.m==1?"PvAI":"AIvsAI")));
    win.textContent = d.o ? (d.w + " WINS!") : "";
    if(d.m==1) aiinfo.textContent = "AI=" + d.j + "%";
    else if(d.m==2) aiinfo.textContent = "AI1=" + d.i + "%  |  AI2=" + d.j + "%";
    else aiinfo.textContent = "";
    const ctx = document.getElementById("game").getContext("2d");
    ctx.clearRect(0,0,W,H);
    ctx.fillStyle="#0f0"; ctx.fillRect(0, 11, W, 1);
    ctx.fillStyle="#123";
    for(let yy=12; yy<H; yy+=6){ ctx.fillRect(W/2-1, yy, 2, 3); }
    ctx.fillStyle="#00ff00";
    ctx.fillRect(0, d.p, PW, PH);
    ctx.fillRect(W-PW, d.q, PW, PH);
    ctx.fillStyle="#0ff";
    ctx.fillRect(d.x, d.y, 2, 2);
  }catch(e){}
}
setInterval(poll, 150); poll();
</script>
</body>
</html>
)HTML";

// ===== Buzzer =====
static unsigned long beepEnd = 0;
inline void beep_start(int freq, int duration){
  tone(BUZZER_PIN, freq);
  beepEnd = millis() + duration;
}
inline void beep_update(){
  if(beepEnd && millis() >= beepEnd){
    noTone(BUZZER_PIN);
    beepEnd = 0;
  }
}

// ===== Game Helpers =====
inline void resetBall(){
  ballX=64; ballY=32;
  ballDX = (random(0,2)?1:-1);
  ballDY = (random(0,2)?1:-1);
}
inline void clampPaddles(){
  if(p1Y<12) p1Y=12; if(p2Y<12) p2Y=12;
  if(p1Y>64-paddleH) p1Y=64-paddleH;
  if(p2Y>64-paddleH) p2Y=64-paddleH;
}
inline void flagWin(Winner w){
  gameOver=true; winner=w; lastWinner=w;
  gameOverTime=millis(); beep_start(1800,200);
}
inline void checkWin(){
  if(p1Score>=10){
    if(gameMode==0) flagWin(W_P1);
    else if(gameMode==1) flagWin(W_YOU);
    else flagWin(W_AI1);
  } else if(p2Score>=10){
    if(gameMode==0) flagWin(W_P2);
    else if(gameMode==1) flagWin(W_AI);
    else flagWin(W_AI2);
  }
}
inline void updateBall(){
  if(gameOver || gameMode==-1) return;
  ballX+=ballDX; ballY+=ballDY;
  if(ballY<=12){ ballY=12; ballDY = abs(ballDY); beep_start(800,30); }
  if(ballY>=63){ ballY=63; ballDY = -abs(ballDY); beep_start(800,30); }
  if(ballX<=paddleW && ballY>=p1Y && ballY<=p1Y+paddleH){ ballX=paddleW; ballDX=abs(ballDX); beep_start(1000,40); }
  if(ballX>=127-paddleW && ballY>=p2Y && ballY<=p2Y+paddleH){ ballX=127-paddleW; ballDX=-abs(ballDX); beep_start(1000,40); }
  if(ballX<0){ p2Score++; resetBall(); checkWin(); }
  if(ballX>127){ p1Score++; resetBall(); checkWin(); }
}
inline void readJoystickP1(){ int y=analogRead(JOY1_Y); if(y<1500)p1Y-=2; else if(y>3000)p1Y+=2; }
inline void readJoystickP2(){ int y=analogRead(JOY2_Y); if(y<1500)p2Y-=2; else if(y>3000)p2Y+=2; }

// ===== Game Modes =====
void playPvP(){ readJoystickP1(); readJoystickP2(); clampPaddles(); updateBall(); }
void playPvAI(){
  readJoystickP1();
  static unsigned long lastMove=0;
  if(millis()-lastMove>50){
    int errorRange = map(100 - ai2Accuracy, 0, 100, 0, 12);
    int targetY = ballY + random(-errorRange, errorRange);
    if(targetY < p2Y + paddleH/2) p2Y -= 2;
    else if(targetY > p2Y + paddleH/2) p2Y += 2;
    lastMove=millis();
  }
  clampPaddles(); updateBall();
}
void playAIvsAI(){
  static unsigned long lastMove1=0,lastMove2=0;
  if(millis()-lastMove1>60){
    int err1 = map(100 - ai1Accuracy, 0, 100, 0, 12);
    int target1 = ballY + random(-err1, err1);
    if(target1 < p1Y + paddleH/2) p1Y -= 2;
    else if(target1 > p1Y + paddleH/2) p1Y += 2;
    lastMove1=millis();   
  }
  if(millis()-lastMove2>60){
    int err2 = map(100 - ai2Accuracy, 0, 100, 0, 12);
    int target2 = ballY + random(-err2, err2);
    if(target2 < p2Y + paddleH/2) p2Y -= 2;
    else if(target2 > p2Y + paddleH/2) p2Y += 2;
    lastMove2=millis();   
  }
  clampPaddles(); updateBall();
}

// ===== OLED Drawing =====
void drawFrame(const char* l,const char* r){
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_5x8_tr);
    if(gameMode==-1){
      u8g2.drawStr(25,30,"PING PONG GAME");
      if(lastWinner!=NONE) u8g2.drawStr(40,50,winnerName(lastWinner));
    }else{
      char buf1[12], buf2[12];
      snprintf(buf1,sizeof(buf1),"%s:%d", l,p1Score);
      snprintf(buf2,sizeof(buf2),"%s:%d", r,p2Score);
      u8g2.drawStr(0,8,buf1);
      u8g2.drawStr(95,8,buf2);
      u8g2.drawHLine(0, 11, 128);
      u8g2.drawBox(0,p1Y,paddleW,paddleH);
      u8g2.drawBox(127-paddleW,p2Y,paddleW,paddleH);
      u8g2.drawBox(ballX,ballY,2,2);
      if(gameOver){
        int w = u8g2.getStrWidth(winnerName(winner));
        u8g2.drawStr((128-w)/2, 30, winnerName(winner));
        w = u8g2.getStrWidth("WINS!");
        u8g2.drawStr((128-w)/2, 45, "WINS!");
      }
    }
  } while(u8g2.nextPage());
}

// ===== AI Accuracy =====
inline void randomizeAIAccuracy(){
  if(gameMode==1){ ai1Accuracy=100; ai2Accuracy=random(1,101); }
  else if(gameMode==2){ ai1Accuracy=random(1,101); ai2Accuracy=random(1,101); }
  else { ai1Accuracy=0; ai2Accuracy=0; }
}

// ===== Web Handlers =====
void handleRoot(){
  server.send_P(200, "text/html", INDEX_HTML);   
}
void handleScore(){
  char buf[128];
  snprintf_P(buf,sizeof(buf),
    PSTR("{\"a\":%d,\"b\":%d,\"m\":%d,\"o\":%s,"
         "\"w\":\"%s\",\"i\":%d,\"j\":%d,"
         "\"x\":%d,\"y\":%d,\"p\":%d,\"q\":%d}"),
    p1Score,p2Score,gameMode,gameOver?"true":"false",
    winnerName(winner), ai1Accuracy, ai2Accuracy,
    ballX, ballY, p1Y, p2Y);
  server.send(200,F("application/json"),buf);
}
void handleReset(){
  p1Score=0; p2Score=0; gameOver=false; winner=NONE;
  resetBall(); randomizeAIAccuracy();
  server.send(200,F("text/plain"),F("OK"));
}
void handleMode(){
  if(server.hasArg("m")){
    int m=server.arg("m").toInt();
    if(m>=0 && m<=2){
      gameMode=m; p1Score=p2Score=0; gameOver=false; winner=NONE;
      resetBall(); randomizeAIAccuracy();
    }
  }
  server.send(200,F("text/plain"),F("Mode set"));
}
void handlePing(){ server.send(200,F("text/plain"),F("pong")); }
void handleBrightness(){
  if(server.hasArg("val")){
    int val = server.arg("val").toInt();
    if(val<0) val=0; if(val>255) val=255;
    oledBrightness = val; u8g2.setContrast(oledBrightness);
  }
  server.send(200,F("text/plain"),F("OK"));
}

// ===== Button debounce =====
bool readButtonOnce(uint8_t pin){
  static uint32_t t1=0,t2=0,t3=0;
  uint32_t now = millis();
  bool pressed = (digitalRead(pin)==LOW);
  uint32_t* tp = (pin==BTN1? &t1 : (pin==BTN2? &t2 : &t3));
  if(pressed && now - *tp > 250){ *tp = now; return true; }
  return false;
}

// ===== Frame pacing =====
inline bool frameReady(){
  static uint32_t last=0;
  uint32_t now = millis();
  if(now - last >= 16){ last = now; return true; }
  return false;
}

// ===== Setup/Loop =====
void setup(){
  u8g2.begin(); u8g2.setContrast(oledBrightness);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BTN1, INPUT_PULLUP);
  pinMode(BTN2, INPUT_PULLUP);
  pinMode(BTN3, INPUT_PULLUP);
  randomSeed((uint32_t)esp_timer_get_time() ^ micros());

  WiFi.mode(WIFI_AP);
  IPAddress ip(192,168,4,1), gw(192,168,4,1), mask(255,255,255,0);
  WiFi.softAPConfig(ip,gw,mask);
  WiFi.softAP(SSID,PASS);

  server.on("/", handleRoot);
  server.on("/score.json", handleScore);
  server.on("/reset", HTTP_POST, handleReset);
  server.on("/mode", handleMode);
  server.on("/ping", handlePing);
  server.on("/brightness", handleBrightness);
  server.begin();

  resetBall();
}

void loop(){
  server.handleClient();
  beep_update();

  if(readButtonOnce(BTN1)){ gameMode=0; p1Score=p2Score=0; gameOver=false; winner=NONE; resetBall(); randomizeAIAccuracy(); }
  if(readButtonOnce(BTN2)){ gameMode=1; p1Score=p2Score=0; gameOver=false; winner=NONE; resetBall(); randomizeAIAccuracy(); }
  if(readButtonOnce(BTN3)){ gameMode=2; p1Score=p2Score=0; gameOver=false; winner=NONE; resetBall(); randomizeAIAccuracy(); }

  if(gameOver && (millis()-gameOverTime>3000)){
    p1Score=0; p2Score=0; gameOver=false; winner=NONE;
    resetBall(); randomizeAIAccuracy();
  }

  if(!frameReady()) return;

  if(!gameOver && gameMode!=-1){
    if(gameMode==0)      playPvP();
    else if(gameMode==1) playPvAI();
    else                 playAIvsAI();
  }

  if(gameMode==0)      drawFrame("P1","P2");
  else if(gameMode==1) drawFrame("YOU","AI");
  else if(gameMode==2) drawFrame("AI1","AI2");
  else                 drawFrame("","");
}
