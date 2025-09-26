#include <U8g2lib.h>
#include <Wire.h>

// OLED SH1106
U8G2_SH1106_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// Joystick P1
#define JOY1_Y  A0
#define JOY1_SW 6   // SW button joystick P1

// Game variables
int p1Y = 30, p2Y = 30;
int ballX = 64, ballY = 32;
int ballDX = 1, ballDY = 1;
const int paddleH = 16, paddleW = 2;
int p1Score = 0, p2Score = 0;
int gameMode = 0;   // 0=PvAI, 1=AIvsAI
bool selectConfirmed = false;

unsigned long lastSw=0;

void setup() {
  u8g2.begin();
  pinMode(JOY1_SW, INPUT_PULLUP);
  drawMenu();
}

void loop() {
  if (!selectConfirmed) {
    selectMode();
  } else {
    if (gameMode == 0) playPvAI();
    else playAIvsAI();
  }
}

// -------- Select Mode --------
void selectMode() {
  int y1 = analogRead(JOY1_Y);

  if (y1 < 400) { 
    gameMode = 0;   // Player vs AI
    drawMenu(); 
    delay(200); 
  }
  else if (y1 > 600) { 
    gameMode = 1;   // AI vs AI
    drawMenu(); 
    delay(200); 
  }

  if (digitalRead(JOY1_SW) == LOW) {
    if (millis() - lastSw > 500) {
      selectConfirmed = true;
      lastSw = millis();
      u8g2.clearBuffer();
      u8g2.setFont(u8g2_font_6x12_tr);
      u8g2.drawStr(20, 30, "Starting...");
      u8g2.sendBuffer();
      delay(1000);
    }
  }
}

void drawMenu() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x12_tr);
  u8g2.drawStr(10, 15, "Select Game Mode");
  u8g2.drawStr(20, 35, gameMode == 0 ? "> Player vs AI" : "  Player vs AI");
  u8g2.drawStr(20, 50, gameMode == 1 ? "> AI vs AI" : "  AI vs AI");
  u8g2.sendBuffer();
}

// -------- Pong Core --------
void resetBall(){ 
  ballX=64; ballY=32; 
  ballDX=(random(0,2)?1:-1); 
  ballDY=random(-1,2); 
}
void updateBall(){
  ballX+=ballDX; ballY+=ballDY;
  if(ballY<=0||ballY>=63) ballDY=-ballDY;
  if(ballX<=paddleW && ballY>=p1Y && ballY<=p1Y+paddleH) ballDX=-ballDX;
  if(ballX>=127-paddleW && ballY>=p2Y && ballY<=p2Y+paddleH) ballDX=-ballDX;
  if(ballX<=0){p2Score++;resetBall();}
  if(ballX>=127){p1Score++;resetBall();}
}
void readJoystickP1(){ 
  int y=analogRead(JOY1_Y); 
  if(y<400&&p1Y>0) p1Y-=2; 
  else if(y>600&&p1Y<64-paddleH) p1Y+=2; 
}

void playPvAI(){
  readJoystickP1();
  if(ballY<p2Y+paddleH/2) p2Y-=2;
  else if(ballY>p2Y+paddleH/2) p2Y+=2;
  updateBall(); drawGame("YOU","AI");
}
void playAIvsAI(){
  if(ballY<p1Y+paddleH/2) p1Y-=2;
  else if(ballY>p1Y+paddleH/2) p1Y+=2;
  if(ballY<p2Y+paddleH/2) p2Y-=2;
  else if(ballY>p2Y+paddleH/2) p2Y+=2;
  updateBall(); drawGame("AI1","AI2");
}

void drawGame(const char* l,const char* r){
  char buf1[12], buf2[12];
  snprintf(buf1,sizeof(buf1),"%s:%d",l,p1Score);
  snprintf(buf2,sizeof(buf2),"%s:%d",r,p2Score);
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_6x12_tr);
    u8g2.drawBox(0,p1Y,paddleW,paddleH);
    u8g2.drawBox(127-paddleW,p2Y,paddleW,paddleH);
    u8g2.drawBox(ballX,ballY,2,2);
    u8g2.drawStr(4,10,buf1);
    u8g2.drawStr(92,10,buf2);
  } while(u8g2.nextPage());
}
