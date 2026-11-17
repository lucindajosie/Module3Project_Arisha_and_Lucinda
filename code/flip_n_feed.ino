#include <WiFi.h>
#include <esp_now.h>
#include <TFT_eSPI.h>

// settings to change
#define TEAM_ID 7
#define PLAYER_ID 1 //0 = host (left plate), 1 = right plate 

#define BUTTON_A 0
#define BUTTON_B 35

#define SCREEN_W 135
#define SCREEN_H 240

// game tuning
#define FRAME_MS 30
#define GRAVITY 0.25
#define MAX_FALL_SPEED 8.0
#define LAUNCH_SPEED 8.1
#define FLIP_TIME_MS 300
#define MAX_MISSES 3
#define HEARTBEAT_MS 100
#define TILT_DEGREES 15
#define SLIDE_ACCEL 0.08
#define MAX_SLIDE_SPEED 2.0
#define MIN_FLIP_POWER 0.65
#define FULL_POWER_ZONE 0.7

// sizes + positions
#define PLATE_W 50
#define PLATE_Y 212
#define LEFT_PLATE_X 4
#define RIGHT_PLATE_X 81
#define DOG_W 18
#define DOG_H 8
#define FACE_Y 72
#define MOUTH_X 67
#define MOUTH_Y 84

const bool IS_HOST = (PLAYER_ID == 0);

uint8_t broadcastAddress[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite frame = TFT_eSprite(&tft);

// game state
float dogX = 0, dogY = -DOG_H;
float dogVX = 0, dogVY = 0;
bool leftUp = false;
bool rightUp = false;
int fed = 0;
int missed = 0;
bool gameOver = false;
int teamFlips = 0;

// host only
uint32_t leftPressedAt = 0;
uint32_t rightPressedAt = 0;
uint32_t gameOverAt = 0;
int partnerPresses = 0;
int dogResting = -1;
float slideSpeed = 0;

// both boards
int myPresses = 0;
uint32_t lastFrame = 0;
uint32_t lastHeartbeat = 0;
bool lastA = HIGH, lastB = HIGH;
uint32_t lastPressTime = 0;
uint32_t chewUntil = 0;
int lastFed = 0;
float leftAngle = TILT_DEGREES;
float rightAngle = -TILT_DEGREES;

// message both boards send
typedef struct {
  uint8_t teamId;
  uint8_t playerId;
  uint16_t presses;
  int16_t dogX;
  int16_t dogY;
  uint8_t leftUp;
  uint8_t rightUp;
  uint8_t fed;
  uint8_t missed;
  uint8_t gameOver;
  uint16_t teamFlips;
} Message;

Message incoming;
volatile bool newMessage = false;

// runs when a message arrives, just save it for loop()
void onReceive(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (len != sizeof(Message)) return;

  Message msg;
  memcpy(&msg, data, sizeof(msg));
  if (msg.teamId != TEAM_ID) return;
  if (msg.playerId == PLAYER_ID) return;

  incoming = msg;
  newMessage = true;
}

void setupEspNow() {
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    tft.setTextColor(TFT_RED);
    tft.drawString("ESP-NOW failed", 5, 100, 2);
    while (true) delay(1000);
  }
  esp_now_register_recv_cb(onReceive);

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, broadcastAddress, 6);
  peer.channel = 0;
  peer.encrypt = false;
  esp_now_add_peer(&peer);
}

void sendMessage() {
  Message msg = {};
  msg.teamId = TEAM_ID;
  msg.playerId = PLAYER_ID;
  msg.presses = myPresses;

  if (IS_HOST) {
    msg.dogX = dogX;
    msg.dogY = dogY;
    msg.leftUp = leftUp;
    msg.rightUp = rightUp;
    msg.fed = fed;
    msg.missed = missed;
    msg.gameOver = gameOver;
    msg.teamFlips = teamFlips;
  }

  esp_now_send(broadcastAddress, (uint8_t *)&msg, sizeof(msg));
}

//Arisha
// game rules (host only)

// drop a new hotdog above a random plate
void spawnHotdog() {
  int plateX;
  if (random(2) == 0) {
    plateX = LEFT_PLATE_X;
  } else {
    plateX = RIGHT_PLATE_X;
  }

  dogX = plateX + random(0, PLATE_W - DOG_W);
  dogY = -DOG_H;
  dogVX = 0;
  dogVY = 0;
  dogResting = -1;
  slideSpeed = 0;
}

void startNewGame() {
  fed = 0;
  missed = 0;
  gameOver = false;
  spawnHotdog();
}

void launchHotdog(bool isLeftPlate);
void plateButtonPressed(bool isLeftPlate) {
  if (gameOver) {
    if (millis() - gameOverAt > 1000) startNewGame();  // wait 1 second before restarting
    return;
  }
  launchHotdog(isLeftPlate);

  if (isLeftPlate) {
    leftPressedAt = millis();
  } else {
    rightPressedAt = millis();
  }
}

// is the hotdog close enough to the mouth? 
bool hotdogReachedMouth() {
  float dx = (dogX + DOG_W / 2) - MOUTH_X;
  float dy = (dogY + DOG_H / 2) - MOUTH_Y;
  return fabs(dx) < 15 && fabs(dy) < 12;
}
// Arisha

// lucinda
void landOnPlate(bool isLeftPlate) {
  if (isLeftPlate) {
    dogResting = 0;
  } else {
    dogResting = 1;
  }

  dogY = PLATE_Y - DOG_H;
  dogVX = 0;
  dogVY = 0;
  slideSpeed = 0;
}
//lucinda

//lucinda
// hotdog slides toward the middle and speeds up
void updateSliding() {
  if (dogResting == -1) return;
  bool isLeft = (dogResting == 0);
  slideSpeed += SLIDE_ACCEL;
  if (slideSpeed > MAX_SLIDE_SPEED) {
    slideSpeed = MAX_SLIDE_SPEED;
  }

  if (isLeft) {
    dogX += slideSpeed;
  } else {
    dogX -= slideSpeed;
  }

  float centerX = dogX + DOG_W / 2.0;

  // follow the plate's tilt so it sits on the plate instead of floating
  float distanceFromPivot;
  if (isLeft) {
    distanceFromPivot = centerX - LEFT_PLATE_X;
  } else {
    distanceFromPivot = (RIGHT_PLATE_X + PLATE_W) - centerX;
  }
  dogY = PLATE_Y + distanceFromPivot * tan(TILT_DEGREES * PI / 180.0) - DOG_H - 4;

  //slide off the tip so it falls 
  bool fellOff = (isLeft && centerX > LEFT_PLATE_X + PLATE_W) || (!isLeft && centerX < RIGHT_PLATE_X);
  if (fellOff) {
    dogResting = -1;
    dogVX = isLeft ? slideSpeed : -slideSpeed;
    dogVY = 1.5;
  }
}
//lucinda

//start lucinda
void launchHotdog(bool isLeftPlate) {
  int plate = isLeftPlate ? 0 : 1;

  // only launch if the hotdog is actually resting on this player's plate
  if (dogResting != plate) return;

  int plateX = isLeftPlate
                 ? LEFT_PLATE_X
                 : RIGHT_PLATE_X;

  float centerX = dogX + DOG_W / 2.0;

  // where is it on the plate? 0 = outer end, 1 = inner tip
  float position;

  if (isLeftPlate) {
    position = (centerX - plateX) / PLATE_W;
  } else {
    position = (plateX + PLATE_W - centerX) / PLATE_W;
  }

  position = constrain(position, 0.0f, 1.0f);

  // inner 30% = full power (goes in the mouth), closer to the outer end = weaker (goes to your partner)
  float power;
  if (position >= FULL_POWER_ZONE) {
    power = 1.0;
  } else {
    power = MIN_FLIP_POWER + (1.0 - MIN_FLIP_POWER) * (position / FULL_POWER_ZONE);
  }

  float speed = LAUNCH_SPEED * power;

  dogResting = -1;
  slideSpeed = 0;

  // launch it up, aimed so the top of the arc lands at the mouth
  dogVY = -speed;

  float framesToTop = speed / GRAVITY;

  dogVX = (MOUTH_X - centerX) / framesToTop;
}
//lucinda

void updateGame() {
  // a plate stays up for a moment after its button is pressed
  leftUp = (millis() - leftPressedAt) < FLIP_TIME_MS;
  rightUp = (millis() - rightPressedAt) < FLIP_TIME_MS;
  teamFlips = myPresses + partnerPresses;

  //Lucinda
  if (gameOver) return;

  if (dogResting != -1) {

    // on a plate: slide
    updateSliding();

  } else {

    // in the air: gravity
    float previousBottom = dogY + DOG_H;

    dogVY += GRAVITY;

    if (dogVY > MAX_FALL_SPEED) {
      dogVY = MAX_FALL_SPEED;
    }

    dogX += dogVX;
    dogY += dogVY;

    float centerX = dogX + DOG_W / 2.0;
    float bottom = dogY + DOG_H;

    // did it just fall past plate height?
    if (dogVY > 0 && previousBottom <= PLATE_Y && bottom >= PLATE_Y) {

      if (centerX >= LEFT_PLATE_X && centerX <= LEFT_PLATE_X + PLATE_W) {

        landOnPlate(true);

        // if the plate is already up, flip it right away
        if (leftUp) {
          launchHotdog(true);
        }

      } else if (centerX >= RIGHT_PLATE_X && centerX <= RIGHT_PLATE_X + PLATE_W) {

        landOnPlate(false);

        if (rightUp) {
          launchHotdog(false);
        }
      }
    }
  }
  // lucinda

  // man eating it
  if (hotdogReachedMouth()) {
    fed++;
    spawnHotdog();
  }

  // fell off the bottom, lose a life
  if (dogY > SCREEN_H) {
    missed++;
    if (missed >= MAX_MISSES) {
      gameOver = true;
      gameOverAt = millis();
    } else {
      spawnHotdog();
    }
  }
}

// messages
void handleMessage() {
  if (IS_HOST) {
    // player 1's press count went up, so flip the right plate
    if (incoming.presses != partnerPresses) {
      partnerPresses = incoming.presses;
      plateButtonPressed(false);
    }
  } else {
    // player 1 just copies the host's game so it can draw it
    dogX = incoming.dogX;
    dogY = incoming.dogY;
    leftUp = incoming.leftUp;
    rightUp = incoming.rightUp;
    fed = incoming.fed;
    missed = incoming.missed;
    gameOver = incoming.gameOver;
    teamFlips = incoming.teamFlips;
  }
}

//Arisha 
// drawing
void drawMan() {
  uint16_t skin = tft.color565(240, 200, 160);
  uint16_t shirt = tft.color565(70, 110, 200);

  // body
  frame.fillRoundRect(MOUTH_X - 55, FACE_Y + 22, 110, 90, 45, shirt);
  frame.fillRect(MOUTH_X - 55, FACE_Y + 67, 110, 45, shirt);

  // head
  frame.fillCircle(MOUTH_X, FACE_Y, 26, skin);

  // eyes
  frame.fillEllipse(MOUTH_X - 8, FACE_Y - 8, 2, 3, TFT_BLACK);
  frame.fillEllipse(MOUTH_X + 8, FACE_Y - 8, 2, 3, TFT_BLACK);

  // mouth closes while he's chewing
  if (millis() < chewUntil) {
    frame.fillRect(MOUTH_X - 8, MOUTH_Y - 1, 16, 3, TFT_BLACK);
  } else {
    frame.fillEllipse(MOUTH_X, MOUTH_Y, 8, 7, TFT_BLACK);
  }
}

void drawHotdog(int x, int y) {
  uint16_t bun = tft.color565(230, 180, 100);
  uint16_t sausage = tft.color565(200, 60, 40);
  frame.fillRoundRect(x, y, DOG_W, DOG_H, 4, bun);
  frame.fillRoundRect(x - 2, y + 2, DOG_W + 4, DOG_H - 4, 2, sausage);
}

// draws a tilted oval by going around it in 32 small steps
void drawTiltedOval(int cx, int cy, int rx, int ry, float angleDegrees, uint16_t color, bool filled) {
  float a = angleDegrees * PI / 180.0;
  int lastX = 0, lastY = 0;

  for (int i = 0; i <= 32; i++) {
    float t = i * 2 * PI / 32;
    float ox = rx * cos(t);
    float oy = ry * sin(t);

    // rotate that point by the tilt angle
    int x = round(cx + ox * cos(a) - oy * sin(a));
    int y = round(cy + ox * sin(a) + oy * cos(a));

    if (i > 0) {
      if (filled) {
        frame.fillTriangle(cx, cy, lastX, lastY, x, y, color);
      } else {
        frame.drawLine(lastX, lastY, x, y, color);
      }
    }
    lastX = x;
    lastY = y;
  }
}

void drawPlate(int x, float angle, bool isUp, bool isLeftPlate, uint16_t color, bool isMine) {
  uint16_t plateWhite = tft.color565(235, 235, 235);
  float a = angle * PI / 180.0;

  // like a pinball flipper, each plate swings from its outer end
  int pivotX;
  int pivotY = PLATE_Y;
  int centerX, centerY;
  if (isLeftPlate) {
    pivotX = x; 
    centerX = round(pivotX + (PLATE_W / 2) * cos(a));
    centerY = round(pivotY + (PLATE_W / 2) * sin(a));
  } else {
    pivotX = x + PLATE_W;
    centerX = round(pivotX - (PLATE_W / 2) * cos(a));
    centerY = round(pivotY - (PLATE_W / 2) * sin(a));
  }

  drawTiltedOval(centerX, centerY, PLATE_W / 2, 7, angle, plateWhite, true);
  drawTiltedOval(centerX, centerY, PLATE_W / 2, 7, angle, color, false);

  // the middle lights up when it's flipped
  if (isUp) {
    drawTiltedOval(centerX, centerY, PLATE_W / 2 - 7, 4, angle, color, true);
  } else {
    drawTiltedOval(centerX, centerY, PLATE_W / 2 - 7, 4, angle, TFT_DARKGREY, false);
  }

  if (isMine) {
    frame.setTextDatum(BC_DATUM);
    frame.setTextColor(TFT_WHITE);
    frame.drawString("YOU", x + PLATE_W / 2, SCREEN_H - 1, 1);
  }
}

void drawGame() {
  frame.fillSprite(TFT_BLACK);

  // if the fed count just went up, close the mouth for a moment
  if (fed > lastFed) chewUntil = millis() + 400;
  lastFed = fed;

  drawMan();

  // resting the plates make a \ / shape, flipped they swing up into / \ 
  float leftTarget, rightTarget;
  if (leftUp) leftTarget = -TILT_DEGREES;
  else leftTarget = TILT_DEGREES;
  if (rightUp) rightTarget = TILT_DEGREES;
  else rightTarget = -TILT_DEGREES;

  // move halfway there each frame so the swing looks smooth
  leftAngle = leftAngle + (leftTarget - leftAngle) * 0.5;
  rightAngle = rightAngle + (rightTarget - rightAngle) * 0.5;

  drawPlate(LEFT_PLATE_X, leftAngle, leftUp, true, TFT_CYAN, IS_HOST);
  drawPlate(RIGHT_PLATE_X, rightAngle, rightUp, false, TFT_ORANGE, !IS_HOST);

  if (!gameOver) drawHotdog(dogX, dogY);

  // score
  frame.setTextColor(TFT_WHITE);
  frame.setTextDatum(TL_DATUM);
  frame.drawString("Fed: " + String(fed), 2, 2, 1);
  frame.setTextDatum(TR_DATUM);
  frame.drawString("Lives: " + String(MAX_MISSES - missed), SCREEN_W - 2, 2, 1);

  if (gameOver) {
    frame.setTextDatum(MC_DATUM);
    frame.setTextColor(TFT_RED);
    frame.drawString("GAME", SCREEN_W / 2, 145, 4);
    frame.drawString("OVER", SCREEN_W / 2, 170, 4);
    frame.setTextColor(TFT_WHITE);
    frame.drawString("Press to replay", SCREEN_W / 2, 195, 2);
  }
// Arisha

  frame.pushSprite(0, 0);
}

// buttons
// true only the moment the button goes down, so one press counts once
bool buttonPressed(uint8_t pin, bool &last) {
  bool now = digitalRead(pin);
  bool pressed = (last == HIGH && now == LOW);
  last = now;
  return pressed;
}

// set up + loop
void setup() {
  Serial.begin(115200);

  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT);

  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);

  frame.setColorDepth(8);
  frame.createSprite(SCREEN_W, SCREEN_H);

  setupEspNow();

  if (IS_HOST) startNewGame();
}

void loop() {
  bool a = buttonPressed(BUTTON_A, lastA);
  bool b = buttonPressed(BUTTON_B, lastB);
  if ((a || b) && millis() - lastPressTime > 20) {
    lastPressTime = millis();
    myPresses++;

    if (IS_HOST) {
      plateButtonPressed(true);  // host flips the left plate
    } else {
      sendMessage();  // player 1 tells the host
    }
  }

  // new message from the other board
  if (newMessage) {
    newMessage = false;
    handleMessage();
  }

  // player 1 keeps resending its presses in case one got lost
  if (!IS_HOST && millis() - lastHeartbeat > HEARTBEAT_MS) {
    lastHeartbeat = millis();
    sendMessage();
  }

  // every frame the host updates and shares the game, and both boards draw
  if (millis() - lastFrame >= FRAME_MS) {
    lastFrame = millis();

    if (IS_HOST) {
      updateGame();
      sendMessage();
    }

    drawGame();
  }
}
