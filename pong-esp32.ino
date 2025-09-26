#include <WiFi.h>
#include <WebServer.h>
#include <U8g2lib.h>
#include <Wire.h>

// ===== OLED SH1106 (Page Buffer) =====
U8G2_SH1106_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// ===== Joystick =====
#define JOY1_Y 34
#define JOY2_Y 35

// ===== Buzzer =====
#define BUZZER_PIN 25

// ===== Buttons =====
#define BTN1 13  // Select PvP
#define BTN2 12  // Select PvAI
#define BTN3 14  // Select AIvsAI

// ===== Game State =====
int p1Y=30, p2Y=30;
int ballX=64, ballY=32;
int ballDX=1, ballDY=1;
const int paddleH=16, paddleW=2;
int p1Score=0, p2Score=0;
int gameMode=-1;
bool gameOver=false;
char winner[8]="";
char lastWinner[8]="";
const int maxScore=10;
unsigned long gameOverTime=0;

// ===== AI Accuracy =====
int ai1Accuracy=0;
int ai2Accuracy=0;

// ===== OLED Brightness =====
int oledBrightness = 128;  // ค่าเริ่มต้น (0–255)

// ===== WiFi AP/Web =====
WebServer server(80);
const char *SSID="ESP32-PONG";
const char *PASS="12345678";

// ===== HTML (Flash) =====
static const char INDEX_HTML[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8"/>
<title>ESP32 Pong</title>
<style>
/* ===== Base ===== */
body {
  font-family: monospace;
  background: #0b0f14;
  color: #eee;
  text-align: center;
  padding: 20px;
  margin: 0;
}

/* ===== Title ===== */
h1 {
  font-size: 28px;
  color: #38bdf8;
  text-shadow: 0 0 6px #0ff;
  margin-bottom: 16px;
}

/* ===== Score ===== */
.score {
  font-size: 28px;
  font-weight: bold;
  margin: 10px 0;
  color: #0ff;
  text-shadow: 0 0 6px #0ff;
}

/* ===== Mode / AI Info ===== */
.mode {
  color: #94a3b8;
  margin-bottom: 4px;
  font-size: 14px;
}
.aiinfo {
  color: #38bdf8;
  font-size: 13px;
  margin-bottom: 12px;
  height: 16px;
  letter-spacing: 1px;
}
.win {
  font-size: 18px;
  color: #0ff;
  margin-top: 10px;
  text-shadow: 0 0 8px #0ff;
}

/* ===== Buttons ===== */
button {
  margin: 6px;
  padding: 10px 16px;
  font-size: 14px;
  font-weight: bold;
  border: 2px solid #38bdf8;
  border-radius: 6px;
  background: #0b1724;
  color: #38bdf8;
  cursor: pointer;
  transition: all 0.2s;
  box-shadow: 0 0 6px rgba(56,189,248,0.5);
}
button:hover {
  background: #38bdf8;
  color: #0b1724;
  box-shadow: 0 0 12px #38bdf8;
}
button:active {
  transform: scale(0.95);
}

/* ===== Brightness Slider ===== */
label {
  font-size: 14px;
  color: #94a3b8;
}
input[type=range] {
  width: 200px;
  margin-top: 10px;
}
</style>
</head>
<body>
<h1>ESP32 Pong</h1>

<!-- Score + Status -->
<div class="score" id="sc">0 - 0</div>
<div class="mode" id="md">Mode: Not Selected</div>
<div class="aiinfo" id="aiinfo"></div>
<div class="win" id="win"></div>

<!-- Mode Buttons -->
<div>
  <p><button onclick="fetch('/mode?m=0')">Player vs Player</button></p>
  <p><button onclick="fetch('/mode?m=1')">Player vs AI</button></p>
  <p><button onclick="fetch('/mode?m=2').then(()=>setTimeout(poll,150))">AI vs AI</button></p>
</div>

<!-- Control Buttons -->
<div>
  <p><button onclick="fetch('/reset',{method:'POST'}).then(()=>setTimeout(poll,150))">Reset</button></p>
  <p><button onclick="fetch('/ping')">Ping</button></p>
  <p><button onclick="location.reload()">Refresh</button></p>
</div>

<!-- Brightness Control -->
<div>
  <label>Brightness:</label><br>
  <input type="range" id="br" min="0" max="255" value="128"
         oninput="fetch('/brightness?val='+this.value)">
</div>

<script>
async function poll(){
  try{
    const r = await fetch('/score.json');
    const d = await r.json();
    sc.textContent = d.P1+" - "+d.P2;
    md.textContent = "Mode: " + (d.mode==-1?"Not Selected":(d.mode==0?"PvP":(d.mode==1?"PvAI":"AIvsAI")));
    win.textContent = d.over ? (d.winner+" WINS!") : "";
    if(d.mode==1) aiinfo.textContent = "AI="+d.AI2+"%";
    else if(d.mode==2) aiinfo.textContent = "AI1="+d.AI1+"%  |  AI2="+d.AI2+"%";
    else aiinfo.textContent="";
  }catch(e){}
}
setInterval(poll,500); poll();
</script>
</body>
</html>
)HTML";

// ===== Buzzer Helper =====
void beep(int freq, int duration){
  tone(BUZZER_PIN, freq, duration);
  delay(duration);
  noTone(BUZZER_PIN);
}

// ===== Game Logic =====
void resetBall(){
  ballX=64; ballY=32;
  ballDX = (random(0,2)?1:-1);
  ballDY = (random(0,2)?1:-1);
}

inline void clampPaddles(){
  if(p1Y<0) p1Y=0; if(p2Y<0) p2Y=0;
  if(p1Y>64-paddleH) p1Y=64-paddleH;
  if(p2Y>64-paddleH) p2Y=64-paddleH;
}

void checkWin(){
  if(p1Score>=maxScore){
    gameOver=true;
    strcpy(winner,(gameMode==0?"P1":gameMode==1?"YOU":"AI1"));
    strcpy(lastWinner,winner);
    gameOverTime=millis();
    beep(1800,300);
  }
  if(p2Score>=maxScore){
    gameOver=true;
    strcpy(winner,(gameMode==0?"P2":gameMode==1?"AI":"AI2"));
    strcpy(lastWinner,winner);
    gameOverTime=millis();
    beep(1800,300);
  }
}

void updateBall(){
  if(gameOver || gameMode==-1) return;

  ballX+=ballDX; 
  ballY+=ballDY;

  if(ballY<=0){ 
    ballY=0; 
    ballDY = abs(ballDY); 
    beep(800,30); 
  }
  if(ballY>=63){ 
    ballY=63; 
    ballDY = -abs(ballDY); 
    beep(800,30); 
  }

  if(ballX<=paddleW && ballY>=p1Y && ballY<=p1Y+paddleH){ 
    ballX = paddleW; 
    ballDX = abs(ballDX); 
    beep(1000,50); 
  }

  if(ballX>=127-paddleW && ballY>=p2Y && ballY<=p2Y+paddleH){ 
    ballX = 127-paddleW; 
    ballDX = -abs(ballDX); 
    beep(1000,50); 
  }

  if(ballX<0){ p2Score++; resetBall(); checkWin(); }
  if(ballX>127){ p1Score++; resetBall(); checkWin(); }
}

void readJoystickP1(){ int y=analogRead(JOY1_Y); if(y<1500)p1Y-=2; else if(y>3000)p1Y+=2; }
void readJoystickP2(){ int y=analogRead(JOY2_Y); if(y<1500)p2Y-=2; else if(y>3000)p2Y+=2; }

void playPvP(){ readJoystickP1(); readJoystickP2(); clampPaddles(); updateBall(); }

void playPvAI(){
  readJoystickP1();
  static unsigned long lastMove=0;
  if(millis()-lastMove>60){
    if(random(0,100)<ai2Accuracy){
      if(ballY<p2Y+paddleH/2)p2Y-=2;
      else if(ballY>p2Y+paddleH/2)p2Y+=2;
    }
    lastMove=millis();
  }
  clampPaddles(); updateBall();
}

void playAIvsAI(){
  static unsigned long lastMove1=0,lastMove2=0;
  if(millis()-lastMove1>80){
    if(random(0,100) < ai1Accuracy){
      if(ballY<p1Y+paddleH/2)p1Y-=2;
      else if(ballY>p1Y+paddleH/2)p1Y+=2;
    }
    lastMove1=millis();
  }
  if(millis()-lastMove2>100){
    if(random(0,100) < ai2Accuracy){
      if(ballY<p2Y+paddleH/2)p2Y-=2;
      else if(ballY>p2Y+paddleH/2)p2Y+=2;
    }
    lastMove2=millis();
  }
  clampPaddles(); updateBall();
}

// ===== OLED Drawing (Font 8px) =====
void drawFrame(const char* l,const char* r){
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_5x8_tr);

    if(gameMode==-1){
      u8g2.drawStr(25,30,"PING PONG GAME");
      if(strlen(lastWinner)>0) u8g2.drawStr(30,50,lastWinner);
    }else{
      char buf1[12], buf2[12];
      snprintf(buf1,sizeof(buf1),"%s:%d", l,p1Score);
      snprintf(buf2,sizeof(buf2),"%s:%d", r,p2Score);

      u8g2.drawStr(0,8,buf1);
      u8g2.drawStr(95,8,buf2);

      u8g2.drawBox(0,p1Y,paddleW,paddleH);
      u8g2.drawBox(127-paddleW,p2Y,paddleW,paddleH);
      u8g2.drawBox(ballX,ballY,2,2);

      if(gameOver){
        int w = u8g2.getStrWidth(winner);
        u8g2.drawStr((128-w)/2, 30, winner);
        w = u8g2.getStrWidth("WINS!");
        u8g2.drawStr((128-w)/2, 45, "WINS!");
      }
    }
  } while(u8g2.nextPage());
}

// ===== Randomize AI Accuracy =====
void randomizeAIAccuracy(){
  if(gameMode==1){          // Player vs AI
    ai1Accuracy = 100;
    ai2Accuracy = random(1,101);
  } else if(gameMode==2){   // AI vs AI
    ai1Accuracy = random(1,101);
    ai2Accuracy = random(1,101);
  } else {
    ai1Accuracy = 0;
    ai2Accuracy = 0;
  }
}

// ===== Web Handlers =====
void handleRoot(){ server.send_P(200,"text/html",INDEX_HTML); }
void handleScore(){
  char buf[128];
  snprintf(buf,sizeof(buf),
    "{\"P1\":%d,\"P2\":%d,\"mode\":%d,\"over\":%s,"
    "\"winner\":\"%s\",\"AI1\":%d,\"AI2\":%d}",
    p1Score,p2Score,gameMode,
    gameOver?"true":"false", winner,
    ai1Accuracy, ai2Accuracy);
  server.send(200,"application/json",buf);
}
void handleReset(){
  p1Score=0; p2Score=0; gameOver=false; winner[0]='\0';
  resetBall();
  randomizeAIAccuracy();
  server.send(200,"text/plain","OK");
}
void handleMode(){
  if(server.hasArg("m")){
    int m=server.arg("m").toInt();
    if(m>=0 && m<=2){
      gameMode=m; p1Score=0; p2Score=0; gameOver=false; winner[0]='\0';
      resetBall();
      randomizeAIAccuracy();
    }
  }
  server.send(200,"text/plain","Mode set");
}
void handlePing(){ server.send(200,"text/plain","pong"); }

void handleBrightness(){
  if(server.hasArg("val")){
    int val = server.arg("val").toInt();
    if(val < 0) val = 0;
    if(val > 255) val = 255;
    oledBrightness = val;
    u8g2.setContrast(oledBrightness);
  }
  server.send(200,"text/plain","OK");
}

// ===== Setup/Loop =====
void setup(){
  u8g2.begin();
  u8g2.setContrast(oledBrightness);
  pinMode(BUZZER_PIN, OUTPUT);

  pinMode(BTN1, INPUT_PULLUP);
  pinMode(BTN2, INPUT_PULLUP);
  pinMode(BTN3, INPUT_PULLUP);

  randomSeed((uint32_t)esp_timer_get_time() ^ micros());

  WiFi.mode(WIFI_AP);
  IPAddress ip(192,168,4,1), gw(192,168,4,1), mask(255,255,255,0);
  WiFi.softAPConfig(ip,gw,mask);
  WiFi.softAP(SSID,PASS);

  server.on("/",handleRoot);
  server.on("/score.json",handleScore);
  server.on("/reset",HTTP_POST,handleReset);
  server.on("/mode",handleMode);
  server.on("/ping",handlePing);
  server.on("/brightness",handleBrightness);
  server.begin();

  resetBall();
}

void loop(){
  server.handleClient();

  // ===== Button Control =====
  if(digitalRead(BTN1)==LOW){
    gameMode=0; p1Score=0; p2Score=0; gameOver=false; winner[0]='\0';
    resetBall(); randomizeAIAccuracy(); delay(300);
  }
  if(digitalRead(BTN2)==LOW){
    gameMode=1; p1Score=0; p2Score=0; gameOver=false; winner[0]='\0';
    resetBall(); randomizeAIAccuracy(); delay(300);
  }
  if(digitalRead(BTN3)==LOW){
    gameMode=2; p1Score=0; p2Score=0; gameOver=false; winner[0]='\0';
    resetBall(); randomizeAIAccuracy(); delay(300);
  }

  if(gameOver && (millis()-gameOverTime>3000)){
    p1Score=0; p2Score=0; gameOver=false; winner[0]='\0';
    resetBall(); randomizeAIAccuracy();
  }

  if(!gameOver && gameMode!=-1){
    if(gameMode==0)      playPvP();
    else if(gameMode==1) playPvAI();
    else                 playAIvsAI();
  }

  if(gameMode==0)      drawFrame("P1","P2");
  else if(gameMode==1) drawFrame("YOU","AI");
  else if(gameMode==2) drawFrame("AI1","AI2");
  else                 drawFrame("","");

  delay(20);
}
