// Portable core of the antenna rotator: axis motion profiles and the text
// command protocol. No Arduino dependencies, so it is unit-tested on a PC
// (see test/host) and reused unchanged by the ESP32 firmware.
#pragma once

#include <string>

namespace rotator {

struct AxisConfig {
  // Soft limits in mechanical degrees. 0 is the servo centre.
  float minDeg;
  float maxDeg;
  float maxSpeedDps;
  float accelDps2;
  // Servo calibration: minUs..maxUs sweeps servoSpanDeg, centred on 0.
  float servoSpanDeg;
  int minUs;
  int maxUs;
  bool invert;
  float trimDeg;
};

class Axis {
 public:
  explicit Axis(const AxisConfig& config);

  // Sets position and target without any motion (used at power-up).
  void reset(float deg);
  void setTarget(float deg);
  // Moves at a constant rate until stop(), a new target, or a soft limit.
  void jog(float rateDps);
  void stop();
  void update(float dt);

  bool inRange(float deg) const;
  float position() const { return pos_; }
  float target() const { return target_; }
  bool moving() const;
  int pulseUs() const;
  const AxisConfig& config() const { return c_; }

 private:
  AxisConfig c_;
  float pos_ = 0;
  float target_ = 0;
  float vel_ = 0;
  float jogRate_ = 0;
  bool jogging_ = false;
};

struct Config {
  AxisConfig pan;
  AxisConfig tilt;
  // A JOG command expires after this long unless it is repeated, so a lost
  // link or a closed browser tab cannot leave the antenna turning.
  float jogTimeoutS;
  // False for a pan-only build (tilt set by hand): elevations are accepted
  // and ignored, and the tilt axis never moves.
  bool hasTilt;
};

class Controller {
 public:
  explicit Controller(const Config& config);

  // Executes one command line and returns "OK ..." or "ERR ...".
  std::string handle(const std::string& line);
  void update(float dt);

  Axis& pan() { return pan_; }
  Axis& tilt() { return tilt_; }
  const Axis& pan() const { return pan_; }
  const Axis& tilt() const { return tilt_; }

  // Compass bearing of the pan axis centre. 0 until NORTH is used, in which
  // case azimuths are simply relative to where the mast faces.
  float northOffset() const { return northOffset_; }
  void setNorthOffset(float deg);
  float azimuth() const;

  std::string status() const;

 private:
  std::string gotoAzEl(float az, float el);

  Axis pan_;
  Axis tilt_;
  float jogTimeoutS_;
  bool hasTilt_;
  float jogRemainingS_ = 0;
  float northOffset_ = 0;
};

float wrap180(float deg);
float wrap360(float deg);

}  // namespace rotator
