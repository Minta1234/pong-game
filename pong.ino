#include <U8g2lib.h>
#include <Wire.h>

// OLED
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// Joysticks
#define JOY1_Y A0
#define JOY1_SW 2
#define JOY2_Y A1
#define JOY2_SW 3

// Game variables
int p1Y = 30, p2Y = 30;
int ballX = 64, ballY = 32;
int ballDX = 1, ballDY = 1;
int paddleHeight = 16, paddleWidth = 2;
int p1Score = 0, p2Score = 0;
int gameMode = 0;          // 0 = PvP, 1 = PvAI
bool selectConfirmed = false;

void setup() {
  u8g2.begin();
  pinMode(JOY1_SW, INPUT_PULLUP);
  pinMode(JOY2_SW, INPUT_PULLUP);
  drawMenu();
}

void loop() {
  if (!selectConfirmed) {
    selectMode();
  } else {
    if (gameMode == 0) playPvP();
    else playPvAI();
  }
}

// -------- Menu select --------
void selectMode() {
  int y1 = analogRead(JOY1_Y);

  if (y1 < 400) { gameMode = 0; drawMenu(); delay(200); }
  else if (y1 > 600) { gameMode = 1; drawMenu(); delay(200); }

  if (digitalRead(JOY1_SW) == LOW) {
    selectConfirmed = true;
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x12_tr);
    u8g2.drawStr(20, 30, "Starting...");
    u8g2.sendBuffer();
    delay(1000);
  }
}

void drawMenu() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x12_tr);
  u8g2.drawStr(10, 15, "Select Game Mode");
  u8g2.drawStr(20, 35, gameMode == 0 ? "> Player vs Player" : "  Player vs Player");
  u8g2.drawStr(20, 50, gameMode == 1 ? "> Player vs AI" : "  Player vs AI");
  u8g2.sendBuffer();
}

// -------- Player vs Player --------
void playPvP() {
  readJoystickP1();
  readJoystickP2();
  updateBall(0); // 0 = PvP
  drawGame("P1", "P2");
}

// -------- Player vs AI --------
void playPvAI() {
  readJoystickP1();

  // AI simple follow
  if (ballY < p2Y + paddleHeight / 2) p2Y -= 2;
  else if (ballY > p2Y + paddleHeight / 2) p2Y += 2;

  updateBall(1); // 1 = PvAI
  drawGame("YOU", "AI");
}

// -------- Input Joysticks --------
void readJoystickP1() {
  int y = analogRead(JOY1_Y);
  if (y < 400 && p1Y > 0) p1Y -= 2;
  else if (y > 600 && p1Y < 64 - paddleHeight) p1Y += 2;
}

void readJoystickP2() {
  int y = analogRead(JOY2_Y);
  if (y < 400 && p2Y > 0) p2Y -= 2;
  else if (y > 600 && p2Y < 64 - paddleHeight) p2Y += 2;
}

// -------- Ball physics --------
void updateBall(int mode) {
  ballX += ballDX;
  ballY += ballDY;

  if (ballY <= 0 || ballY >= 63) ballDY = -ballDY;

  // Collision P1
  if (ballX <= paddleWidth &&
      ballY >= p1Y && ballY <= p1Y + paddleHeight) {
    ballDX = -ballDX;
    ballDY += (ballY - (p1Y + paddleHeight / 2)) / 4; // add spin
  }

  // Collision P2
  if (ballX >= 127 - paddleWidth &&
      ballY >= p2Y && ballY <= p2Y + paddleHeight) {
    ballDX = -ballDX;
    ballDY += (ballY - (p2Y + paddleHeight / 2)) / 4; // add spin
  }

  // Out of bounds
  if (ballX <= 0) { p2Score++; resetBall(); }
  if (ballX >= 127) { p1Score++; resetBall(); }
}

// -------- Reset ball --------
void resetBall() {
  ballX = 64; ballY = 32;
  ballDX = (random(0, 2) == 0) ? -1 : 1;
  ballDY = random(-1, 2);
}

// -------- Draw game --------
void drawGame(const char* leftName, const char* rightName) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x12_tr);

  // paddles
  u8g2.drawBox(0, p1Y, paddleWidth, paddleHeight);
  u8g2.drawBox(127 - paddleWidth, p2Y, paddleWidth, paddleHeight);

  // ball
  u8g2.drawBox(ballX, ballY, 2, 2);

  // scores
  u8g2.drawStr(5, 10, leftName);
  u8g2.drawStr(35, 10, String(p1Score).c_str());
  u8g2.drawStr(90, 10, rightName);
  u8g2.drawStr(115, 10, String(p2Score).c_str());

  u8g2.sendBuffer();
}
