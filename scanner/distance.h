#pragma once
#include <stdint.h>

extern bool  hasAvg;
extern float rssiAvg;
extern float txPower;
extern float nFactor;

void  updateRssiAvg(int rssi);
float estimateDistanceMeters(float rssi, float txPower, float n);
