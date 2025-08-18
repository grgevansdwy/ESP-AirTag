#include <math.h>
#include "distance.h"

// ---- REAL DEFINITIONS (exactly once) ----
bool  hasAvg  = false;
float rssiAvg = 0.0f;
float txPower = -59.0f;   // tune later
float nFactor = 2.5f;     // tune later

void updateRssiAvg(int rssi) {
  const float alpha = 0.2f;
  if (!hasAvg) { rssiAvg = rssi; hasAvg = true; }
  else         { rssiAvg = alpha*rssi + (1.0f - alpha)*rssiAvg; }
}

float estimateDistanceMeters(float rssi, float txPower, float n) {
  return powf(10.0f, (txPower - rssi) / (10.0f * n));
}
