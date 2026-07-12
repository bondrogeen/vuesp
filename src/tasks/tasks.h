#ifndef TASKS_H
#define TASKS_H

#include "./eeprom/eeprom.h"
#include "./files/files.h"
#include "./init/init.h"
#include "./webserver/webserver.h"

#define MESSAGE_TYPE_LOG 0
#define MESSAGE_TYPE_NOTIFICATION 1
#define MESSAGE_TYPE_SCRIPT 2

void loopTask(uint32_t now);
void onWsEventTasks(void* arg, uint8_t* data, size_t len, uint32_t clientId, uint8_t task);
void sendNotificationLog(const char* message);
void sendNotification();
extern uint8_t tasks[];
extern Port port;

#endif