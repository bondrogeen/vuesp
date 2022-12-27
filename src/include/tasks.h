#ifndef TASKS_H
#define TASKS_H

#include "./init.h"
#include "./webserver.h"

void loopTask(uint32_t now);
void onWsEvent(void *arg, uint8_t *data, size_t len, uint32_t clientId);
extern uint8_t tasks[];

#endif