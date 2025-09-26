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
const int maxScore=5;
unsigned long gameOverTime=0;

// ===== WiFi AP/Web =====
WebServer server(80);
const char *SSID="ESP32-PONG";
const char *PASS="12345678";

// ===== HTML (เก็บใน Flash) =====
static const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head><meta charset="utf-8"/>
<title>ESP32 Pong</title>
<style>
body{font-family:sans-serif;background:#0b0f14;color:#eee;text-align:center;padding:20px}
button{margin:6px;padding:10px 16px;font-size:16px;border:0;border-radius:6px;cursor:pointer}
.score{font-size:32px;font-weight:bold;margin:12px 0}
.mode{color:#94a3b8;margin-bottom:12px}
.win{font-size:20px;color:#0ff;margin-top:10px}
</style></head><body>
<h1>ESP32 Pong</h1>
<div class="score" id="sc">0 - 0</div>
<div class="mode" id="md">Mode: Not Selected</div>
<div class="win" id="win"></div>
<div>
  <button onclick="fetch('/mode?m=0')">Player vs Player</button>
  <button onclick="fetch('/mode?m=1')">Player vs AI</button>
  <button onclick="fetch('/mode?m=2')">AI vs AI</button>
</div>
<div>
  <button onclick="fetch('/reset',{method:'POST'})">Reset</button>
  <button onclick="fetch('/ping')">Ping</button>
  <button onclick="location.reload()">Refresh</button>
</div>
<script>
async function poll(){
  try{
    const r = await fetch('/score.json');
    const d = await r.json();
    sc.textContent = d.P1+" - "+d.P2;
    md.textContent = "Mode: " + (d.mode==-1?"Not Selected":(d.mode==0?"PvP":(d.mode==1?"PvAI":"AIvsAI")));
    win.textContent = d.over ? (d.winner+" WINS!") : "";
  }catch(e){}
}
setInterval(poll,500); poll();
</script>
</body></html>
)rawliteral";

// ===== Buzzer Helper =====
void beep(int freq, int duration){
  tone(BUZZER_PIN, freq, duration);
  delay(duration);
  noTone(BUZZER_PIN);
}

// ===== Game Logic =====
void resetBall(){ 
  ballX=64; ballY=32; 
  ballDX=(random(0,2)?1:-1); 
  ballDY=random(-1,2); 
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
    beep(1800,300); // เสียงชนะ
  }
  if(p2Score>=maxScore){ 
    gameOver=true; 
    strcpy(winner,(gameMode==0?"P2":gameMode==1?"AI":"AI2"));
    strcpy(lastWinner,winner);
    gameOverTime=millis();
    beep(1800,300); // เสียงชนะ
  }
}
void updateBall(){
  if(gameOver || gameMode==-1) return;
  ballX+=ballDX; ballY+=ballDY;

  // ชนขอบบน/ล่าง
  if(ballY<=0||ballY>=63){
    ballDY=-ballDY;
    beep(800,30); // bounce wall
  }

  // ชน Paddle
  if(ballX<=paddleW && ballY>=p1Y && ballY<=p1Y+paddleH){ 
    ballDX=-ballDX; 
    beep(1000,50); 
  }
  if(ballX>=127-paddleW && ballY>=p2Y && ballY<=p2Y+paddleH){ 
    ballDX=-ballDX; 
    beep(1000,50); 
  }

  // ออกนอกจอ
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
    if(ballY<p2Y+paddleH/2)p2Y-=2; 
    else if(ballY>p2Y+paddleH/2)p2Y+=2; 
    lastMove=millis();
  }
  clampPaddles(); updateBall(); 
}
void playAIvsAI(){
  static unsigned long lastMove1=0,lastMove2=0;
  if(millis()-lastMove1>80){ 
    if(random(0,100)>20){ 
      if(ballY<p1Y+paddleH/2)p1Y-=2;
      else if(ballY>p1Y+paddleH/2)p1Y+=2;
    }
    lastMove1=millis();
  }
  if(millis()-lastMove2>100){ 
    if(random(0,100)>50){ 
      if(ballY<p2Y+paddleH/2)p2Y-=2;
      else if(ballY>p2Y+paddleH/2)p2Y+=2;
    }
    lastMove2=millis();
  }
  clampPaddles(); updateBall();
}

void drawFrame(const char* l,const char* r){
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_6x12_tr);
    if(gameMode==-1){
      u8g2.drawStr(20,30,"PING PONG GAME");
      if(strlen(lastWinner)>0) u8g2.drawStr(15,50,lastWinner);
    }else{
      char buf1[12], buf2[12];
      snprintf(buf1,sizeof(buf1),"%s:%d", l,p1Score);
      snprintf(buf2,sizeof(buf2),"%s:%d", r,p2Score);

      // วาด Paddle + Ball
      u8g2.drawBox(0,p1Y,paddleW,paddleH);
      u8g2.drawBox(127-paddleW,p2Y,paddleW,paddleH);
      u8g2.drawBox(ballX,ballY,2,2);

      // วาด Score
      u8g2.drawStr(4,10,buf1);
      u8g2.drawStr(92,10,buf2);

      // ถ้าเกมจบ → แสดงผลผู้ชนะตรงกลาง
      if(gameOver){
        int w = u8g2.getStrWidth(winner);
        u8g2.drawStr((128-w)/2, 30, winner);

        w = u8g2.getStrWidth("WINS!");
        u8g2.drawStr((128-w)/2, 45, "WINS!");
      }
    }
  } while(u8g2.nextPage());
}

// ===== Web Handlers =====
void handleRoot(){ server.send_P(200,"text/html",INDEX_HTML); }
void handleScore(){
  char buf[96];
  snprintf(buf,sizeof(buf),
    "{\"P1\":%d,\"P2\":%d,\"mode\":%d,\"over\":%s,\"winner\":\"%s\"}",
    p1Score,p2Score,gameMode, gameOver?"true":"false", winner);
  server.send(200,"application/json",buf);
}
void handleReset(){ p1Score=0;p2Score=0;gameOver=false;winner[0]='\0';resetBall(); server.send(200,"text/plain","OK"); }
void handleMode(){
  if(server.hasArg("m")){
    int m=server.arg("m").toInt();
    if(m>=0&&m<=2){ gameMode=m; p1Score=0;p2Score=0;gameOver=false;winner[0]='\0'; resetBall(); }
  }
  server.send(200,"text/plain","Mode set");
}
void handlePing(){ server.send(200,"text/plain","pong"); }

// ===== Setup/Loop =====
void setup(){
  u8g2.begin();
  pinMode(BUZZER_PIN,OUTPUT);
  WiFi.mode(WIFI_AP);
  IPAddress ip(192,168,4,1),gw(192,168,4,1),mask(255,255,255,0);
  WiFi.softAPConfig(ip,gw,mask);
  WiFi.softAP(SSID,PASS);
  server.on("/",handleRoot);
  server.on("/score.json",handleScore);
  server.on("/reset",HTTP_POST,handleReset);
  server.on("/mode",handleMode);
  server.on("/ping",handlePing);
  server.begin();
  resetBall();
}
void loop(){
  server.handleClient();

  // auto reset หลังชนะ 3 วิ
  if(gameOver && (millis()-gameOverTime>3000)){
    p1Score=0;p2Score=0;gameOver=false;winner[0]='\0';
    resetBall();
  }

  if(!gameOver && gameMode!=-1){
    if(gameMode==0) playPvP();
    else if(gameMode==1) playPvAI();
    else playAIvsAI();
  }

  if(gameMode==0) drawFrame("P1","P2");
  else if(gameMode==1) drawFrame("YOU","AI");
  else if(gameMode==2) drawFrame("AI1","AI2");
  else drawFrame("","");

  delay(20); 
}
