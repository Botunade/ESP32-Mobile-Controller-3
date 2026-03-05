#ifndef DIGITAL_FOLLOWER_H
#define DIGITAL_FOLLOWER_H

#include "webserver.h"

void digitalFollower_init();
void digitalFollower_update(const SystemState& state, const SystemSettings& settings);

#endif
