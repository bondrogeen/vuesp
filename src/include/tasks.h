#ifndef TASKS_H
#define TASKS_H

#include "./init.h"
#include "./webserver.h"

void loopTask(uint32_t now);
void onWsEvent(void *arg, uint8_t *data, size_t len, uint32_t clientId);
void send(uint8_t *message, size_t len, uint8_t task);
extern uint8_t tasks[];

#endif