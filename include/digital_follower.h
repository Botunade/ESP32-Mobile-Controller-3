#ifndef DIGITAL_FOLLOWER_H
#define DIGITAL_FOLLOWER_H

#include "pressure_webserver.h"

void digitalFollower_init();
void digitalFollower_update(const SystemState& state, const SystemSettings& settings);

#endif
