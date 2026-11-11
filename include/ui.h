#pragma once
#include <Arduino.h>

typedef void (*RelaisCallback)(int relais, bool state);
typedef void (*ScenarioCallback)(const char *scenario);

void uiInit(RelaisCallback relaisCb, ScenarioCallback scenarioCb);
void uiUpdateCurrent(float amps);
void uiUpdateRelais(int index, bool state);
