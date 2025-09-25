#include <WiFi.h>
#include <WebServer.h>
#include <U8g2lib.h>

// ========== OLED ==========
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// ========== Joystick ==========
#define JOY1_Y 34
#define JOY1_SW 25
#define JOY2_Y 35
#define JOY2_SW 26

// ========== Game Variables ==========
int p1Y = 30, p2Y = 30;
int ballX = 64, ballY = 32;
int ballDX = 1, ballDY = 1;
int paddleHeight = 16, paddleWidth = 2;
int p1Score = 0, p2Score = 0;
int gameMode = 0;          // 0 = PvP, 1 = PvAI
bool selectConfirmed = false;

// ========== WiFi WebServer ==========
WebServer server(80);
const char *ssid = "ESP32-PONG";
const char *password = "12345678";

// ========== Setup ==========
void setup() {
  Serial.begin(115200);

  // OLED
  u8g2.begin();

  // Joysticks
  pinMode(JOY1_SW, INPUT_PULLUP);
  pinMode(JOY2_SW, INPUT_PULLUP);

  // WiFi SoftAP
  WiFi.softAP(ssid, password);
  Serial.println("AP IP: " + WiFi.softAPIP().toString());

  // Web Routes
  server.on("/", handleRoot);
  server.on("/score", handleScore);
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

// ========== Web Handlers ==========
void handleRoot() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta http-equiv='refresh' content='1'>";
  html += "<title>ESP32 Pong Scoreboard</title></head><body style='font-family:monospace;text-align:center;'>";
  html += "<h1>ESP32 Pong Scoreboard</h1>";
  html += "<h2>P1: " + String(p1Score) + " - P2: " + String(p2Score) + "</h2>";
  html += "<p>Mode: " + String(gameMode == 0 ? "PvP" : "PvAI") + "</p>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleScore() {
  String json = "{\"P1\":" + String(p1Score) + ",\"P2\":" + String(p2Score) + "}";
  server.send(200, "application/json", json);
}

// ========== Menu ==========
void selectMode() {
  int y1 = analogRead(JOY1_Y);
  if (y1 < 1500) { gameMode = 0; drawMenu(); delay(200); }
  else if (y1 > 3000) { gameMode = 1; drawMenu(); delay(200); }

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
  u8g2.drawStr(20, 15, "Select Game Mode");
  u8g2.drawStr(25, 35, gameMode == 0 ? "> Player vs Player" : "  Player vs Player");
  u8g2.drawStr(25, 50, gameMode == 1 ? "> Player vs AI" : "  Player vs AI");
  u8g2.sendBuffer();
}

// ========== Game Modes ==========
void playPvP() {
  readJoystickP1();
  readJoystickP2();
  updateBall();
  drawGame("P1", "P2");
}

void playPvAI() {
  readJoystickP1();
  if (ballY < p2Y + paddleHeight / 2) p2Y -= 2;
  else if (ballY > p2Y + paddleHeight / 2) p2Y += 2;
  updateBall();
  drawGame("YOU", "AI");
}

// ========== Joystick Inputs ==========
void readJoystickP1() {
  int y = analogRead(JOY1_Y);
  if (y < 1500 && p1Y > 0) p1Y -= 2;
  else if (y > 3000 && p1Y < 64 - paddleHeight) p1Y += 2;
}

void readJoystickP2() {
  int y = analogRead(JOY2_Y);
  if (y < 1500 && p2Y > 0) p2Y -= 2;
  else if (y > 3000 && p2Y < 64 - paddleHeight) p2Y += 2;
}

// ========== Ball Physics ==========
void updateBall() {
  ballX += ballDX;
  ballY += ballDY;

  if (ballY <= 0 || ballY >= 63) ballDY = -ballDY;

  if (ballX <= paddleWidth &&
      ballY >= p1Y && ballY <= p1Y + paddleHeight) {
    ballDX = -ballDX;
    ballDY += (ballY - (p1Y + paddleHeight / 2)) / 4;
  }
  if (ballX >= 127 - paddleWidth &&
      ballY >= p2Y && ballY <= p2Y + paddleHeight) {
    ballDX = -ballDX;
    ballDY += (ballY - (p2Y + paddleHeight / 2)) / 4;
  }

  if (ballX <= 0) { p2Score++; resetBall(); }
  if (ballX >= 127) { p1Score++; resetBall(); }
}

void resetBall() {
  ballX = 64; ballY = 32;
  ballDX = (random(0, 2) == 0) ? -1 : 1;
  ballDY = random(-1, 2);
}

// ========== Draw OLED ==========
void drawGame(const char* leftName, const char* rightName) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x12_tr);
  u8g2.drawBox(0, p1Y, paddleWidth, paddleHeight);
  u8g2.drawBox(127 - paddleWidth, p2Y, paddleWidth, paddleHeight);
  u8g2.drawBox(ballX, ballY, 2, 2);
  u8g2.drawStr(5, 10, leftName);
  u8g2.drawStr(35, 10, String(p1Score).c_str());
  u8g2.drawStr(90, 10, rightName);
  u8g2.drawStr(115, 10, String(p2Score).c_str());
  u8g2.sendBuffer();
}
