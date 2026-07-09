#ifndef DISCOVERY_H
#define DISCOVERY_H

#include <WiFiUdp.h>

#include "./init/init.h"
#include "./protocol/DeviceProtocol.h"
#include "./runner/runner.h"

#define UDP_PORT 12345
#define UDP_IP {255, 255, 255, 255}

void setupDiscovery();
void loopDiscovery(uint32_t now);

#endif