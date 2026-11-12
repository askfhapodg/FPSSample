// Host unit tests: `make test`.
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>

#include "rotator_core.h"

using namespace rotator;

static int failures = 0;
#define CHECK(cond)                                                  \
  do {                                                               \
    if (!(cond)) {                                                   \
      std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);    \
      ++failures;                                                    \
    }                                                                \
  } while (0)

static bool near(float a, float b, float eps = 0.05f) { return std::fabs(a - b) <= eps; }
static bool starts(const std::string& s, const char* p) { return s.rfind(p, 0) == 0; }

static Config cfg() {
  AxisConfig pan{-130, 130, 30, 60, 270, 500, 2500, false, 0};
  AxisConfig tilt{-15, 60, 20, 40, 270, 500, 2500, false, 0};
  return Config{pan, tilt, 0.6f};
}

static const float kDt = 0.02f;

static void run(Controller& c, float seconds) {
  for (float t = 0; t < seconds; t += kDt) c.update(kDt);
}

static void testReachesTargetWithinLimits() {
  Controller c(cfg());
  CHECK(starts(c.handle("PAN 90"), "OK"));
  float prev = 0, prevVel = 0, maxVel = 0, maxAcc = 0;
  for (int i = 0; i < 400; ++i) {
    c.update(kDt);
    float vel = (c.pan().position() - prev) / kDt;
    maxVel = std::fmax(maxVel, std::fabs(vel));
    // The final landing step may snap a few hundredths of a degree.
    if (c.pan().moving()) maxAcc = std::fmax(maxAcc, std::fabs(vel - prevVel) / kDt);
    CHECK(c.pan().position() <= 90.001f);  // never overshoots
    prev = c.pan().position();
    prevVel = vel;
  }
  CHECK(near(c.pan().position(), 90, 0.001f));
  CHECK(!c.pan().moving());
  CHECK(maxVel <= 30.01f);
  CHECK(maxVel > 29.0f);
  CHECK(maxAcc <= 60.5f);
}

static void testShortMoveAndReverse() {
  Controller c(cfg());
  c.handle("PAN 0.5");
  run(c, 2);
  CHECK(near(c.pan().position(), 0.5f, 0.001f));
  c.handle("PAN 100");
  run(c, 1);
  c.handle("PAN -100");  // reverse mid-move
  run(c, 15);
  CHECK(near(c.pan().position(), -100, 0.001f));
  CHECK(!c.pan().moving());
}

static void testRangeChecks() {
  Controller c(cfg());
  CHECK(starts(c.handle("PAN 131"), "ERR"));
  CHECK(starts(c.handle("TILT -16"), "ERR"));
  CHECK(starts(c.handle("GOTO 170 0"), "ERR"));
  CHECK(starts(c.handle("GOTO 10 70"), "ERR"));
  run(c, 1);
  CHECK(near(c.pan().position(), 0) && near(c.tilt().position(), 0));
}

static void testPulseMapping() {
  Config k = cfg();
  Axis a(k.pan);
  CHECK(a.pulseUs() == 1500);
  a.reset(90);
  CHECK(a.pulseUs() == 1500 + 667);  // 90/270 * 2000
  k.pan.invert = true;
  k.pan.trimDeg = 2.7f;
  Axis b(k.pan);
  b.reset(90);
  CHECK(b.pulseUs() == 1500 - 667 + 20);
  k.pan.minDeg = -200;
  k.pan.invert = false;
  Axis d(k.pan);
  d.reset(-200);
  CHECK(d.pulseUs() == 500);  // never commands beyond the servo's pulse range
}

static void testJogDeadmanAndLimits() {
  Controller c(cfg());
  CHECK(starts(c.handle("jog 20 0"), "OK"));  // case-insensitive
  run(c, 0.5f);
  CHECK(c.pan().position() > 3);
  run(c, 3);  // no repeat: the deadman must have stopped it
  float p = c.pan().position();
  CHECK(!c.pan().moving());
  CHECK(p < 20);
  run(c, 1);
  CHECK(near(c.pan().position(), p, 0.001f));

  // Held jog (repeated) runs into the soft limit and stops there.
  for (int i = 0; i < 600; ++i) {
    if (i % 10 == 0) c.handle("JOG 100 100");
    c.update(kDt);
    CHECK(c.pan().position() <= 130.001f && c.tilt().position() <= 60.001f);
  }
  CHECK(near(c.pan().position(), 130, 0.2f));
  CHECK(near(c.tilt().position(), 60, 0.2f));

  c.handle("JOG -20 0");
  run(c, 0.2f);
  c.handle("STOP");
  run(c, 2);
  CHECK(!c.pan().moving());
}

static void testNorthAndGoto() {
  Controller c(cfg());
  c.handle("PAN 30");
  run(c, 5);
  CHECK(starts(c.handle("NORTH 350"), "OK"));  // pointing at 350 while pan=30
  CHECK(near(c.northOffset(), 320));
  CHECK(near(c.azimuth(), 350));
  CHECK(starts(c.handle("GOTO 10 20"), "OK"));  // crosses north: pan 30 -> 50
  run(c, 5);
  CHECK(near(c.pan().position(), 50));
  CHECK(near(c.tilt().position(), 20));
  CHECK(near(c.azimuth(), 10));
  CHECK(starts(c.handle("GOTO 140 0"), "ERR"));  // pan would be 180
  CHECK(c.status().find("az=10.0") != std::string::npos);
}

static void testParser() {
  Controller c(cfg());
  CHECK(starts(c.handle(""), "ERR"));
  CHECK(starts(c.handle("   "), "ERR"));
  CHECK(starts(c.handle("FLY"), "ERR"));
  CHECK(starts(c.handle("PAN"), "ERR"));
  CHECK(starts(c.handle("PAN abc"), "ERR"));
  CHECK(starts(c.handle("PAN nan"), "ERR"));
  CHECK(starts(c.handle("PAN inf"), "ERR"));
  CHECK(starts(c.handle("GOTO 1 2 3"), "ERR"));
  CHECK(starts(c.handle("STOP 1"), "ERR"));
  CHECK(starts(c.handle("  status\r"), "OK"));
  CHECK(starts(c.handle("AVERYLONGCOMMANDNAMETHATOVERFLOWS 1"), "ERR"));
  CHECK(near(wrap180(190), -170) && near(wrap180(-190), 170) && near(wrap360(-10), 350));
}

int main() {
  testReachesTargetWithinLimits();
  testShortMoveAndReverse();
  testRangeChecks();
  testPulseMapping();
  testJogDeadmanAndLimits();
  testNorthAndGoto();
  testParser();
  if (failures) {
    std::printf("%d check(s) failed\n", failures);
    return 1;
  }
  std::printf("all tests passed\n");
  return 0;
}
