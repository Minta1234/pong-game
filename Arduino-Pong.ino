#include <U8g2lib.h>
#include <Wire.h>

// ===== OLED SH1106 I2C (A4=SDA, A5=SCL) =====
U8G2_SH1106_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// ===== Pins =====
#define JOY1_Y   A0  // P1 joystick
#define JOY2_Y   A1  // P2 joystick
#define BTN1      2  // P1 SW → Navigate menu
#define BTN2      3  // P2 SW → Select / Reset
#define BUZZER    8  // Buzzer (optional)

// ===== Game State =====
int p1Y=24, p2Y=24;
int ballX=64, ballY=32;
int ballDX=1, ballDY=1;
const int paddleH=16, paddleW=2;
int p1Score=0, p2Score=0;
int gameMode=-1;
int menuSelect=0;
bool gameOver=false;
unsigned long gameOverTime=0;

uint8_t ai1Accuracy=0, ai2Accuracy=0;

enum Winner {NONE,W_P1,W_P2,W_YOU,W_AI,W_AI1,W_AI2};
Winner winner=NONE, lastWinner=NONE;

const char* winnerName(Winner w){
  switch(w){
    case W_P1:  return "P1";
    case W_P2:  return "P2";
    case W_YOU: return "YOU";
    case W_AI:  return "AI";
    case W_AI1: return "AI1";
    case W_AI2: return "AI2";
    default:    return "";
  }
}

// ===== Buzzer =====
static unsigned long beepEnd=0;
void beep_start(int freq, int dur){ tone(BUZZER,freq); beepEnd=millis()+dur; }
void beep_update(){ if(beepEnd && millis()>=beepEnd){ noTone(BUZZER); beepEnd=0; } }

// ===== Button Debounce =====
static uint32_t db1=0, db2=0;
bool readBtn(uint8_t pin){
  uint32_t now=millis();
  uint32_t* tp=(pin==BTN1)?&db1:&db2;
  if(digitalRead(pin)==LOW && now-*tp>250){ *tp=now; return true; }
  return false;
}

// ===== Game Helpers =====
void resetBall(){
  ballX=64; ballY=32;
  ballDX=(random(0,2)?1:-1);
  ballDY=(random(0,2)?1:-1);
}
void clampPaddles(){
  if(p1Y<12) p1Y=12; if(p2Y<12) p2Y=12;
  if(p1Y>64-paddleH) p1Y=64-paddleH;
  if(p2Y>64-paddleH) p2Y=64-paddleH;
}
void flagWin(Winner w){
  gameOver=true; winner=w; lastWinner=w;
  gameOverTime=millis(); beep_start(1800,200);
}
void checkWin(){
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
void updateBall(){
  if(gameOver || gameMode==-1) return;
  ballX+=ballDX; ballY+=ballDY;
  if(ballY<=11){ ballY=11; ballDY=abs(ballDY); beep_start(800,30); }
  if(ballY>=63){ ballY=63; ballDY=-abs(ballDY); beep_start(800,30); }
  if(ballX<=paddleW+1 && ballX>=0 && ballY>=p1Y && ballY<=p1Y+paddleH){
    ballX=paddleW+1; ballDX=abs(ballDX); beep_start(1000,40);
  }
  if(ballX>=125-paddleW && ballX<=127 && ballY>=p2Y && ballY<=p2Y+paddleH){
    ballX=125-paddleW; ballDX=-abs(ballDX); beep_start(1000,40);
  }
  if(ballX<0){ p2Score++; resetBall(); checkWin(); }
  if(ballX>127){ p1Score++; resetBall(); checkWin(); }
}

// ===== Joystick (Arduino ADC 0-1023) =====
void readJoystickP1(){ int y=analogRead(JOY1_Y); if(y<400)p1Y-=2; else if(y>600)p1Y+=2; }
void readJoystickP2(){ int y=analogRead(JOY2_Y); if(y<400)p2Y-=2; else if(y>600)p2Y+=2; }

// ===== Smart AI Prediction =====
int predictBallY(int bx,int by,int dx,int dy,int targetX){
  int px=bx,py=by,pdx=dx,pdy=dy,steps=300;
  while(px!=targetX && steps-->0){
    px+=pdx; py+=pdy;
    if(py<=11){ py=11; pdy=abs(pdy); }
    if(py>=63){ py=63; pdy=-abs(pdy); }
  }
  return py;
}

void randomizeAI(){
  if(gameMode==1){ ai1Accuracy=100; ai2Accuracy=random(40,101); }
  else if(gameMode==2){ ai1Accuracy=random(40,101); ai2Accuracy=random(40,101); }
  else { ai1Accuracy=0; ai2Accuracy=0; }
}

// ===== Game Modes =====
void playPvP(){ readJoystickP1(); readJoystickP2(); clampPaddles(); updateBall(); }

void playPvAI(){
  readJoystickP1();
  static unsigned long lastMove=0;
  if(millis()-lastMove>50){
    int predicted=predictBallY(ballX,ballY,ballDX,ballDY,125);
    int err=map(100-ai2Accuracy,0,100,0,12);
    int target=predicted+random(-err,err+1);
    int spd=map(100-ai2Accuracy,0,100,4,1);
    if(target<p2Y+paddleH/2) p2Y=max(p2Y-spd,12);
    else if(target>p2Y+paddleH/2) p2Y=min(p2Y+spd,64-paddleH);
    lastMove=millis();
  }
  clampPaddles(); updateBall();
}

void playAIvsAI(){
  static unsigned long lm1=0,lm2=0;
  if(millis()-lm1>60){
    int pred1=predictBallY(ballX,ballY,ballDX,ballDY,2);
    int err1=map(100-ai1Accuracy,0,100,0,12);
    int t1=pred1+random(-err1,err1+1);
    int s1=map(100-ai1Accuracy,0,100,4,1);
    if(t1<p1Y+paddleH/2) p1Y=max(p1Y-s1,12);
    else if(t1>p1Y+paddleH/2) p1Y=min(p1Y+s1,64-paddleH);
    lm1=millis();
  }
  if(millis()-lm2>60){
    int pred2=predictBallY(ballX,ballY,ballDX,ballDY,125);
    int err2=map(100-ai2Accuracy,0,100,0,12);
    int t2=pred2+random(-err2,err2+1);
    int s2=map(100-ai2Accuracy,0,100,4,1);
    if(t2<p2Y+paddleH/2) p2Y=max(p2Y-s2,12);
    else if(t2>p2Y+paddleH/2) p2Y=min(p2Y+s2,64-paddleH);
    lm2=millis();
  }
  clampPaddles(); updateBall();
}

// ===== Drawing =====
void drawMenu(){
  const char* modes[]={"Player vs Player","Player vs AI","AI vs AI"};
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_5x8_tr);
    u8g2.drawStr(28,8,"PONG GAME");
    u8g2.drawHLine(0,10,128);
    // Last winner
    if(lastWinner!=NONE){
      char buf[16];
      snprintf(buf,sizeof(buf),"Last: %s",winnerName(lastWinner));
      u8g2.drawStr(0,62,buf);
    }
    // Menu items
    for(int i=0;i<3;i++){
      int yy=22+i*15;
      if(i==menuSelect){
        u8g2.drawRBox(0,yy-9,128,12,2);
        u8g2.setDrawColor(0);
        u8g2.drawStr(4,yy,modes[i]);
        u8g2.setDrawColor(1);
      } else {
        u8g2.drawStr(4,yy,modes[i]);
      }
    }
    // Hint
    u8g2.setFont(u8g2_font_4x6_tr);
    u8g2.drawStr(0,62,"P1:Nav  P2:OK");
  } while(u8g2.nextPage());
}

void drawGame(const char* l, const char* r){
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_5x8_tr);
    char b1[12],b2[12];
    snprintf(b1,sizeof(b1),"%s:%d",l,p1Score);
    snprintf(b2,sizeof(b2),"%s:%d",r,p2Score);
    u8g2.drawStr(0,8,b1);
    u8g2.drawStr(95,8,b2);
    u8g2.drawHLine(0,11,128);
    // Center dashes
    for(int y=14;y<64;y+=6) u8g2.drawBox(63,y,2,3);
    u8g2.drawBox(0,p1Y,paddleW,paddleH);
    u8g2.drawBox(127-paddleW,p2Y,paddleW,paddleH);
    u8g2.drawBox(ballX,ballY,2,2);
    if(gameOver){
      // Overlay box
      u8g2.setDrawColor(0);
      u8g2.drawBox(24,18,80,30);
      u8g2.setDrawColor(1);
      u8g2.drawRFrame(24,18,80,30,3);
      int w=u8g2.getStrWidth(winnerName(winner));
      u8g2.drawStr((128-w)/2,30,winnerName(winner));
      w=u8g2.getStrWidth("WINS!");
      u8g2.drawStr((128-w)/2,43,"WINS!");
    }
  } while(u8g2.nextPage());
}

// ===== Frame Pacing =====
bool frameReady(){
  static uint32_t last=0;
  uint32_t now=millis();
  if(now-last>=16){ last=now; return true; }
  return false;
}

void startGame(int mode){
  gameMode=mode; p1Score=p2Score=0;
  p1Y=p2Y=24; gameOver=false; winner=NONE;
  resetBall(); randomizeAI();
}

// ===== Setup/Loop =====
void setup(){
  u8g2.begin();
  pinMode(BTN1,INPUT_PULLUP);
  pinMode(BTN2,INPUT_PULLUP);
  pinMode(BUZZER,OUTPUT);
  randomSeed(analogRead(A2)^micros());
  resetBall();
}

void loop(){
  beep_update();

  // ===== MENU =====
  if(gameMode==-1){
    if(readBtn(BTN1)){ menuSelect=(menuSelect+1)%3; beep_start(600,30); }
    if(readBtn(BTN2)){ startGame(menuSelect); beep_start(1000,60); }
    drawMenu();
    return;
  }

  // ===== IN GAME =====
  // กด BTN2 หลัง gameOver → กลับ Menu
  if(gameOver && readBtn(BTN2)){ gameMode=-1; winner=NONE; return; }

  // Auto reset หลัง 3 วิ
  if(gameOver && millis()-gameOverTime>3000){ gameMode=-1; winner=NONE; return; }

  if(!frameReady()) return;

  if(!gameOver){
    if(gameMode==0) playPvP();
    else if(gameMode==1) playPvAI();
    else playAIvsAI();
  }

  if(gameMode==0) drawGame("P1","P2");
  else if(gameMode==1) drawGame("YOU","AI");
  else drawGame("AI1","AI2");
}
