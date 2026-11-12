// Antenna rotator firmware for an ESP32 DevKit driving two hobby servos.
//
// Control inputs, all speaking the same text protocol (see README):
//   * USB serial, 115200 baud, one command per line
//   * Wi-Fi access point with a web page and POST /api/cmd
//   * optional wired joystick
#include <Arduino.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>

#include "config.h"
#include "rotator_core.h"
#include "web_ui.h"

namespace {

rotator::Controller controller(rotatorConfig());
WebServer server(80);
Preferences prefs;

const uint32_t kTickMs = 20;  // one servo frame
const int kPwmHz = 50;
const int kPwmBits = 16;
const int kPanChannel = 0;
const int kTiltChannel = 1;

String serialLine;
bool wasMoving = false;
float savedPan = 0, savedTilt = 0, savedNorth = 0;

void servoAttach(int pin, int channel) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  (void)channel;
  ledcAttach(pin, kPwmHz, kPwmBits);
#else
  ledcSetup(channel, kPwmHz, kPwmBits);
  ledcAttachPin(pin, channel);
#endif
}

void servoWrite(int pin, int channel, int pulseUs) {
  uint32_t duty = (uint32_t)pulseUs * ((1u << kPwmBits) - 1) / (1000000u / kPwmHz);
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  (void)channel;
  ledcWrite(pin, duty);
#else
  (void)pin;
  ledcWrite(channel, duty);
#endif
}

void writeServos() {
  servoWrite(PIN_SERVO_PAN, kPanChannel, controller.pan().pulseUs());
  servoWrite(PIN_SERVO_TILT, kTiltChannel, controller.tilt().pulseUs());
}

// Remembers where the antenna points so the next power-up starts from the
// same pulse widths instead of swinging to centre. Written only when a move
// ends, which keeps flash wear negligible.
void persistIfChanged() {
  float pan = controller.pan().position();
  float tilt = controller.tilt().position();
  float north = controller.northOffset();
  if (fabsf(pan - savedPan) > 0.2f) prefs.putFloat("pan", savedPan = pan);
  if (fabsf(tilt - savedTilt) > 0.2f) prefs.putFloat("tilt", savedTilt = tilt);
  if (north != savedNorth) prefs.putFloat("north", savedNorth = north);
}

String run(const String& line) {
  String reply = controller.handle(line.c_str()).c_str();
  if (controller.northOffset() != savedNorth) persistIfChanged();
  return reply;
}

void handleCmd() {
  if (!server.hasArg("c")) {
    server.send(400, "text/plain", "ERR missing parameter c");
    return;
  }
  server.send(200, "text/plain", run(server.arg("c")));
}

void pollSerial() {
  while (Serial.available()) {
    char ch = (char)Serial.read();
    if (ch == '\n' || ch == '\r') {
      if (serialLine.length()) Serial.println(run(serialLine));
      serialLine = "";
    } else if (serialLine.length() < 64) {
      serialLine += ch;
    }
  }
}

#if HAS_JOYSTICK
// Returns -1..1 with a dead zone around the stick's rest position.
float readStick(int pin) {
  float v = (analogRead(pin) - 2048) / 2048.0f;
  const float dead = 0.15f;
  if (fabsf(v) < dead) return 0;
  return (v - copysignf(dead, v)) / (1 - dead);
}

void pollJoystick() {
  static bool active = false;
  float x = readStick(PIN_JOY_X);
  float y = readStick(PIN_JOY_Y);
  bool pressed = digitalRead(PIN_JOY_BUTTON) == LOW;
  if (pressed) {
    controller.handle("STOP");
    active = false;
  } else if (x != 0 || y != 0) {
    controller.pan().jog(x * controller.pan().config().maxSpeedDps);
    controller.tilt().jog(y * controller.tilt().config().maxSpeedDps);
    active = true;
  } else if (active) {
    // Only on release, so an idle stick never cancels Wi-Fi/serial commands.
    controller.handle("STOP");
    active = false;
  }
}
#endif

}  // namespace

void setup() {
  Serial.begin(115200);

  prefs.begin("rotator", false);
  savedPan = prefs.getFloat("pan", 0);
  savedTilt = prefs.getFloat("tilt", 0);
  savedNorth = prefs.getFloat("north", 0);
  controller.pan().reset(savedPan);
  controller.tilt().reset(savedTilt);
  controller.setNorthOffset(savedNorth);

  servoAttach(PIN_SERVO_PAN, kPanChannel);
  servoAttach(PIN_SERVO_TILT, kTiltChannel);
  writeServos();

#if HAS_JOYSTICK
  pinMode(PIN_JOY_BUTTON, INPUT_PULLUP);
#endif

  WiFi.mode(WIFI_AP);
  WiFi.softAP(WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL);
  server.on("/", HTTP_GET, []() { server.send_P(200, "text/html", kIndexHtml); });
  server.on("/api/cmd", HTTP_ANY, handleCmd);
  server.begin();

  Serial.printf("Rotator ready. Wi-Fi \"%s\", http://%s/\n", WIFI_SSID,
                WiFi.softAPIP().toString().c_str());
}

void loop() {
  static uint32_t lastTick = millis();

  server.handleClient();
  pollSerial();

  uint32_t now = millis();
  uint32_t elapsed = now - lastTick;
  if (elapsed < kTickMs) return;
  lastTick = now;

#if HAS_JOYSTICK
  pollJoystick();
#endif
  // Cap dt so a stalled loop cannot turn into one large jump.
  controller.update(min(elapsed, (uint32_t)100) / 1000.0f);
  writeServos();

  bool moving = controller.pan().moving() || controller.tilt().moving();
  if (wasMoving && !moving) persistIfChanged();
  wasMoving = moving;
}
