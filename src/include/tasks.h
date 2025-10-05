#ifndef TASKS_H
#define TASKS_H

#include "./init.h"
#include "./webserver.h"

#define NOTIF_COLOR_TRANSPARENT 0
#define NOTIF_COLOR_GREEN 1
#define NOTIF_COLOR_RED 2
#define NOTIF_COLOR_YELLOW 3
#define NOTIF_COLOR_BLUE 4

void loopTask(uint32_t now);
void onWsEventTasks(void* arg, uint8_t* data, size_t len, uint32_t clientId, uint8_t task);
void send(uint8_t* message, size_t len, uint8_t task);
void sendAll(uint8_t* message, size_t len, uint8_t task);
void sendNotificationText(const char* message, uint8_t color);
void sendNotification();
extern uint8_t tasks[];
extern Port port;

#endif