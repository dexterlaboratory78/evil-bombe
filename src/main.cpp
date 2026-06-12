#include <Arduino.h>
#include <Bluepad32.h>
#include "esp_camera.h"
#include "WebSerial.h"
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

// ════════════════════════════════════════════════════════════
// ░░░ MOTOR CONTROL PINS (TB6612FNG) ░░░
// ════════════════════════════════════════════════════════════

// Left Motor (Motor 1)
#define LEFT_MOTOR_AIN1 GPIO_NUM_40   // GPIO40 → AIN1
#define LEFT_MOTOR_AIN2 GPIO_NUM_41   // GPIO41 → AIN2
#define LEFT_MOTOR_PWMA GPIO_NUM_42   // GPIO42 → PWMA (speed control)

// Right Motor (Motor 2)
#define RIGHT_MOTOR_BIN1 GPIO_NUM_48  // GPIO48 → AIN1 (TB6612 #2)
#define RIGHT_MOTOR_BIN2 GPIO_NUM_45  // GPIO45 → AIN2 (TB6612 #2)
#define RIGHT_MOTOR_PWMB GPIO_NUM_45  // GPIO45 → PWMB (speed control)

// Standby pin (same for both motors)
#define MOTOR_STBY GPIO_NUM_38        // GPIO38 → STBY (enable)

// PWM Configuration
#define PWM_FREQ 5000
#define PWM_RES 8
#define PWM_CH_LEFT 0
#define PWM_CH_RIGHT 1

// ════════════════════════════════════════════════════════════
// ░░░ MOTOR SPEED & STATE ░░░
// ════════════════════════════════════════════════════════════

uint8_t motor_speed = 200;  // 0-255 (default medium speed)
bool auto_spin_active = false;
unsigned long auto_spin_start = 0;
const unsigned long AUTO_SPIN_DURATION = 3000;  // 3 second 360° spin

// ════════════════════════════════════════════════════════════
// ░░░ GAMEPAD STATE ░░░
// ════════════════════════════════════════════════════════════

ControllerPtr myControllers[BP32_MAX_GAMEPADS];

void onConnectedController(ControllerPtr ctl) {
  bool found = false;
  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
    if (myControllers[i] == nullptr) {
      myControllers[i] = ctl;
      found = true;
      Serial.printf("[GAMEPAD] Controller #%d connected\n", i);
      break;
    }
  }
  if (!found) {
    Serial.println("[GAMEPAD] No slot available");
  }
}

void onDisconnectedController(ControllerPtr ctl) {
  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
    if (myControllers[i] == ctl) {
      myControllers[i] = nullptr;
      Serial.printf("[GAMEPAD] Controller #%d disconnected\n", i);
      break;
    }
  }
}

// ════════════════════════════════════════════════════════════
// ░░░ MOTOR CONTROL FUNCTIONS ░░░
// ════════════════════════════════════════════════════════════

void motorInit() {
  // Configure PWM channels
  ledcSetup(PWM_CH_LEFT, PWM_FREQ, PWM_RES);
  ledcSetup(PWM_CH_RIGHT, PWM_FREQ, PWM_RES);
  
  // Attach PWM to GPIO pins
  ledcAttachPin(LEFT_MOTOR_PWMA, PWM_CH_LEFT);
  ledcAttachPin(RIGHT_MOTOR_PWMB, PWM_CH_RIGHT);
  
  // Set direction pins as OUTPUT
  pinMode(LEFT_MOTOR_AIN1, OUTPUT);
  pinMode(LEFT_MOTOR_AIN2, OUTPUT);
  pinMode(RIGHT_MOTOR_BIN1, OUTPUT);
  pinMode(RIGHT_MOTOR_BIN2, OUTPUT);
  pinMode(MOTOR_STBY, OUTPUT);
  
  // Enable motors (STBY HIGH)
  digitalWrite(MOTOR_STBY, HIGH);
  
  // Start stopped
  stopMotors();
  Serial.println("[MOTOR] TB6612FNG initialized");
}

void setLeftMotor(int speed, bool forward) {
  // speed: -255 to 255 (negative = backward)
  if (speed == 0) {
    digitalWrite(LEFT_MOTOR_AIN1, LOW);
    digitalWrite(LEFT_MOTOR_AIN2, LOW);
    ledcWrite(PWM_CH_LEFT, 0);
  } else if (speed > 0) {
    // Forward
    digitalWrite(LEFT_MOTOR_AIN1, HIGH);
    digitalWrite(LEFT_MOTOR_AIN2, LOW);
    ledcWrite(PWM_CH_LEFT, constrain(speed, 0, 255));
  } else {
    // Backward
    digitalWrite(LEFT_MOTOR_AIN1, LOW);
    digitalWrite(LEFT_MOTOR_AIN2, HIGH);
    ledcWrite(PWM_CH_LEFT, constrain(-speed, 0, 255));
  }
}

void setRightMotor(int speed, bool forward) {
  if (speed == 0) {
    digitalWrite(RIGHT_MOTOR_BIN1, LOW);
    digitalWrite(RIGHT_MOTOR_BIN2, LOW);
    ledcWrite(PWM_CH_RIGHT, 0);
  } else if (speed > 0) {
    // Forward
    digitalWrite(RIGHT_MOTOR_BIN1, HIGH);
    digitalWrite(RIGHT_MOTOR_BIN2, LOW);
    ledcWrite(PWM_CH_RIGHT, constrain(speed, 0, 255));
  } else {
    // Backward
    digitalWrite(RIGHT_MOTOR_BIN1, LOW);
    digitalWrite(RIGHT_MOTOR_BIN2, HIGH);
    ledcWrite(PWM_CH_RIGHT, constrain(-speed, 0, 255));
  }
}

void stopMotors() {
  setLeftMotor(0, true);
  setRightMotor(0, true);
}

void driveForward(uint8_t speed) {
  setLeftMotor(speed, true);
  setRightMotor(speed, true);
  Serial.printf("[MOTOR] Drive forward at speed %d\n", speed);
}

void driveBackward(uint8_t speed) {
  setLeftMotor(-speed, false);
  setRightMotor(-speed, false);
  Serial.printf("[MOTOR] Drive backward at speed %d\n", speed);
}

void turnLeft(uint8_t speed) {
  // Left motor slower, right motor faster → turns left
  setLeftMotor(speed / 2, true);
  setRightMotor(speed, true);
  Serial.printf("[MOTOR] Turn left at speed %d\n", speed);
}

void turnRight(uint8_t speed) {
  // Right motor slower, left motor faster → turns right
  setLeftMotor(speed, true);
  setRightMotor(speed / 2, true);
  Serial.printf("[MOTOR] Turn right at speed %d\n", speed);
}

void spinInPlace(bool clockwise, uint8_t speed) {
  if (clockwise) {
    // Right motor forward, left motor backward → spin clockwise
    setLeftMotor(-speed, false);
    setRightMotor(speed, true);
    Serial.printf("[MOTOR] Spin clockwise at speed %d\n", speed);
  } else {
    // Left motor forward, right motor backward → spin counter-clockwise
    setLeftMotor(speed, true);
    setRightMotor(-speed, false);
    Serial.printf("[MOTOR] Spin counter-clockwise at speed %d\n", speed);
  }
}

// ════════════════════════════════════════════════════════════
// ░░░ GAMEPAD PROCESSING ░░░
// ════════════════════════════════════════════════════════════

void processGamepad(ControllerPtr ctl) {
  if (!ctl->isConnected()) return;

  // ── D-Pad: Basic movement ──
  if (ctl->dpadDown()) {
    driveBackward(motor_speed);
  } else if (ctl->dpadUp()) {
    driveForward(motor_speed);
  } else if (ctl->dpadLeft()) {
    turnLeft(motor_speed);
  } else if (ctl->dpadRight()) {
    turnRight(motor_speed);
  }

  // ── Left Stick: Analog drive ──
  int16_t stickY = ctl->axisY();  // -512 to 511
  int16_t stickX = ctl->axisX();  // -512 to 511

  if (abs(stickY) > 50 || abs(stickX) > 50) {
    auto_spin_active = false;
    
    if (abs(stickY) > abs(stickX)) {
      // Primarily vertical movement
      if (stickY > 100) {
        driveForward(map(stickY, 100, 511, 100, 255));
      } else if (stickY < -100) {
        driveBackward(map(-stickY, 100, 511, 100, 255));
      }
    } else {
      // Primarily horizontal movement (turning)
      if (stickX > 100) {
        turnRight(map(stickX, 100, 511, 100, 255));
      } else if (stickX < -100) {
        turnLeft(map(-stickX, 100, 511, 100, 255));
      }
    }
  }

  // ── Button A: Stop ──
  if (ctl->a()) {
    stopMotors();
    auto_spin_active = false;
    Serial.println("[INPUT] A pressed → STOP");
  }

  // ── Button B: Spin right (clockwise) ──
  if (ctl->b()) {
    spinInPlace(true, motor_speed);
    Serial.println("[INPUT] B pressed → Spin right");
  }

  // ── Button X: Spin left (counter-clockwise) ──
  if (ctl->x()) {
    spinInPlace(false, motor_speed);
    Serial.println("[INPUT] X pressed → Spin left");
  }

  // ── Button Y: 360° auto-spin ──
  if (ctl->y()) {
    auto_spin_active = true;
    auto_spin_start = millis();
    Serial.println("[INPUT] Y pressed → 360° auto-spin START");
  }

  // ── Shoulder buttons: Speed control ──
  if (ctl->r1() || ctl->rb()) {
    motor_speed = min(255, motor_speed + 10);
    Serial.printf("[INPUT] Speed up → %d/255\n", motor_speed);
    delay(100);
  }
  if (ctl->l1() || ctl->lb()) {
    motor_speed = max(50, motor_speed - 10);
    Serial.printf("[INPUT] Speed down → %d/255\n", motor_speed);
    delay(100);
  }

  // ── Auto-spin timer ──
  if (auto_spin_active) {
    if (millis() - auto_spin_start < AUTO_SPIN_DURATION) {
      spinInPlace(true, motor_speed);
    } else {
      auto_spin_active = false;
      stopMotors();
      Serial.println("[INPUT] 360° auto-spin COMPLETE");
    }
  }
}

// ════════════════════════════════════════════════════════════
// ░░░ CAMERA INITIALIZATION ░░░
// ════════════════════════════════════════════════════════════

void cameraInit() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_VGA;
  config.jpeg_quality = 10;
  config.fb_count = 1;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("[CAMERA] Init failed with error 0x%x\n", err);
    return;
  }
  Serial.println("[CAMERA] OV2640 initialized successfully");
}

// ════════════════════════════════════════════════════════════
// ░░░ SETUP ░░░
// ════════════════════════════════════════════════════════════

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\n╔════════════════════════════════════════╗");
  Serial.println("║         🐙 EVIL BOMBE v1.0 🐙          ║");
  Serial.println("║    Arachnobeest Walking Octopod        ║");
  Serial.println("╚════════════════════════════════════════╝\n");

  // Initialize motor system
  motorInit();

  // Initialize camera
  cameraInit();

  // Initialize Bluepad32 gamepad
  BP32.setup(&onConnectedController, &onDisconnectedController);
  Serial.println("[GAMEPAD] Bluepad32 initialized - waiting for controller...");

  Serial.println("[SETUP] All systems ready!\n");
}

// ════════════════════════════════════════════════════════════
// ░░░ MAIN LOOP ░░░
// ════════════════════════════════════════════════════════════

void loop() {
  // Update Bluepad32
  BP32.update();

  // Process connected gamepads
  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
    ControllerPtr ctl = myControllers[i];
    if (ctl && ctl->isConnected()) {
      processGamepad(ctl);
    }
  }

  delay(50);  // Process at ~20 Hz
}
