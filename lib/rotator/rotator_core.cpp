#include "rotator_core.h"

#include <cctype>
#include <cmath>
#include <cstdio>

namespace rotator {

namespace {

float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

const float kPosEps = 0.01f;
const float kVelEps = 0.001f;

}  // namespace

float wrap360(float deg) {
  float r = std::fmod(deg, 360.0f);
  return r < 0 ? r + 360.0f : r;
}

float wrap180(float deg) {
  float r = wrap360(deg);
  return r > 180.0f ? r - 360.0f : r;
}

Axis::Axis(const AxisConfig& config) : c_(config) {}

void Axis::reset(float deg) {
  pos_ = target_ = clampf(deg, c_.minDeg, c_.maxDeg);
  vel_ = 0;
  jogging_ = false;
}

bool Axis::inRange(float deg) const { return deg >= c_.minDeg && deg <= c_.maxDeg; }

void Axis::setTarget(float deg) {
  target_ = clampf(deg, c_.minDeg, c_.maxDeg);
  jogging_ = false;
}

void Axis::jog(float rateDps) {
  jogRate_ = clampf(rateDps, -c_.maxSpeedDps, c_.maxSpeedDps);
  jogging_ = true;
}

void Axis::stop() { jog(0); }

bool Axis::moving() const {
  return std::fabs(vel_) > kVelEps || std::fabs(target_ - pos_) > kPosEps;
}

void Axis::update(float dt) {
  if (dt <= 0) return;
  const float maxDv = c_.accelDps2 * dt;
  float desired;
  float err = 0;

  if (jogging_) {
    desired = jogRate_;
    // Slow down so the axis arrives at a soft limit at zero speed.
    if (desired != 0) {
      float dist = desired > 0 ? c_.maxDeg - pos_ : pos_ - c_.minDeg;
      float vmax = std::sqrt(2 * c_.accelDps2 * (dist > 0 ? dist : 0));
      if (std::fabs(desired) > vmax) desired = std::copysign(vmax, desired);
    }
  } else {
    err = target_ - pos_;
    if (std::fabs(err) < kPosEps && std::fabs(vel_) <= maxDv) {
      pos_ = target_;
      vel_ = 0;
      return;
    }
    float v = std::sqrt(2 * c_.accelDps2 * std::fabs(err));
    if (v > c_.maxSpeedDps) v = c_.maxSpeedDps;
    desired = std::copysign(v, err);
  }

  vel_ += clampf(desired - vel_, -maxDv, maxDv);
  pos_ += vel_ * dt;

  if (jogging_) {
    if (jogRate_ == 0 && std::fabs(vel_) <= kVelEps) {
      vel_ = 0;
      jogging_ = false;
    }
    target_ = pos_;
  } else if ((target_ - pos_) * err < 0) {
    // Stepped past the target within this tick: land on it.
    pos_ = target_;
    vel_ = 0;
  }

  if (pos_ < c_.minDeg || pos_ > c_.maxDeg) {
    pos_ = clampf(pos_, c_.minDeg, c_.maxDeg);
    vel_ = 0;
    if (jogging_) target_ = pos_;
  }
}

int Axis::pulseUs() const {
  float deg = (c_.invert ? -pos_ : pos_) + c_.trimDeg;
  float mid = (c_.minUs + c_.maxUs) / 2.0f;
  float us = mid + deg / c_.servoSpanDeg * (c_.maxUs - c_.minUs);
  return static_cast<int>(std::lround(clampf(us, c_.minUs, c_.maxUs)));
}

Controller::Controller(const Config& config)
    : pan_(config.pan),
      tilt_(config.tilt),
      jogTimeoutS_(config.jogTimeoutS),
      hasTilt_(config.hasTilt) {}

void Controller::setNorthOffset(float deg) { northOffset_ = wrap360(deg); }

float Controller::azimuth() const { return wrap360(pan_.position() + northOffset_); }

void Controller::update(float dt) {
  if (jogRemainingS_ > 0) {
    jogRemainingS_ -= dt;
    if (jogRemainingS_ <= 0) {
      pan_.stop();
      tilt_.stop();
    }
  }
  pan_.update(dt);
  tilt_.update(dt);
}

std::string Controller::status() const {
  char buf[176];
  std::snprintf(buf, sizeof(buf),
                "OK az=%.1f el=%.1f pan=%.1f tilt=%.1f north=%.1f moving=%d tiltaxis=%d",
                azimuth(), tilt_.position(), pan_.position(), tilt_.position(), northOffset_,
                (pan_.moving() || tilt_.moving()) ? 1 : 0, hasTilt_ ? 1 : 0);
  return buf;
}

std::string Controller::gotoAzEl(float az, float el) {
  float mech = wrap180(az - northOffset_);
  char buf[120];
  if (!pan_.inRange(mech)) {
    std::snprintf(buf, sizeof(buf), "ERR azimuth out of reach (pan would be %.1f, limits %.1f..%.1f)",
                  mech, pan_.config().minDeg, pan_.config().maxDeg);
    return buf;
  }
  if (hasTilt_ && !tilt_.inRange(el)) {
    std::snprintf(buf, sizeof(buf), "ERR elevation out of range (limits %.1f..%.1f)",
                  tilt_.config().minDeg, tilt_.config().maxDeg);
    return buf;
  }
  jogRemainingS_ = 0;
  pan_.setTarget(mech);
  if (hasTilt_) tilt_.setTarget(el);
  return status();
}

std::string Controller::handle(const std::string& line) {
  char cmd[12] = {0};
  float a = 0, b = 0;
  char extra = 0;
  int n = std::sscanf(line.c_str(), " %11s %f %f %c", cmd, &a, &b, &extra);
  if (n < 1) return "ERR empty command";
  if (n > 3) return "ERR too many arguments";
  int args = n - 1;
  if ((args >= 1 && !std::isfinite(a)) || (args >= 2 && !std::isfinite(b)))
    return "ERR bad number";
  std::string c(cmd);
  for (char& ch : c) ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));

  if (c == "STATUS" && args == 0) return status();

  if (c == "STOP" && args == 0) {
    jogRemainingS_ = 0;
    pan_.stop();
    tilt_.stop();
    return status();
  }
  if (c == "HOME" && args == 0) {
    jogRemainingS_ = 0;
    pan_.setTarget(0);
    tilt_.setTarget(0);
    return status();
  }
  if (c == "GOTO" && args == 2) return gotoAzEl(a, b);
  if (c == "PAN" && args == 1) {
    if (!pan_.inRange(a)) return "ERR pan out of range";
    pan_.setTarget(a);
    return status();
  }
  if (c == "TILT" && args == 1) {
    if (!hasTilt_) return "ERR no tilt axis (tilt is set by hand)";
    if (!tilt_.inRange(a)) return "ERR tilt out of range";
    tilt_.setTarget(a);
    return status();
  }
  if (c == "JOG" && args == 2) {
    pan_.jog(a);
    tilt_.jog(hasTilt_ ? b : 0);
    jogRemainingS_ = jogTimeoutS_;
    return status();
  }
  if (c == "NORTH" && args == 1) {
    // "The antenna is pointing at compass bearing <a> right now."
    setNorthOffset(a - pan_.position());
    return status();
  }
  return "ERR unknown command or wrong argument count";
}

}  // namespace rotator
