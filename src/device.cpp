#include <SPI.h>
#include <SdFat.h>

#include "./include/gpio.h"
#include "./include/init.h"
#include "./include/tasks.h"

SdFat sd;
SdFile dirFile;
SdFile file;

// Number of files found.
uint16_t numberOfFiles = 0;
uint16_t dirIndex[300];

#define SD_CS_PIN 5
File myFile;

const int chipSelect = D8;  // use D0 for Wemos D1 Mini
File root;

uint32_t lastTimeDevice = 0;

Device device = {KEY_DEVICE, 0, 0, 0, ""};

void onWsEventDevice(void *arg, uint8_t *data, size_t len, uint32_t clientId) {
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len) {
    if (info->opcode == WS_BINARY) {
      uint8_t task = data[0];
      if (task == KEY_DEVICE && info->len == sizeof(device)) {
        memcpy(&device, data, sizeof(device));
        tasks[task] = true;
      }
    }
  }
}

void onSend(uint8_t direction) {
  device.example1 += 1;
  device.direction = direction;
  send((uint8_t *)&device, sizeof(device), KEY_DEVICE);
}

// void getFile(char *name) {
//   Dir dir = LittleFS.openDir(name);
//   while (dir.next()) {
//     memset(files.name, 0, sizeof(files.name));
//     dir.fileName().toCharArray(files.name, 32);
//     files.size = dir.fileSize();
//     files.isFile = dir.isFile();
//     files.isDir = dir.isDirectory();
//     wsSend((uint8_t *)&files, sizeof(files));
//   }
//   tasks[KEY_FILES] = false;
// }

void printDirectory(SdFile CFile, int numTabs) {
  SdFile file;
  while (file.openNext(&CFile, O_READ)) {
    if (file.isSubDir()) {
      SdFile SubDirFile;
      file.printName(&Serial);
      Serial.println();
      if (SubDirFile.open(&CFile, file.dirIndex(), O_READ)) {
        printDirectory(SubDirFile, numTabs + 1);
      }
    } else {
      file.printName(&Serial);
      Serial.println();
    }
  }
  file.close();
}

void setupDevice() {
  File dir;

  if (!sd.begin(SD_CS_PIN)) {
    Serial.println("initialization failed!");
    return;
  } else {
    Serial.println("initialization!");
    sd.ls("/", LS_R);
    // if (dirFile.open("/", O_READ)) {
    //   printDirectory(dirFile, 0);

    // }
  }
}

void loopDevice(uint32_t now) {
  if (now - lastTimeDevice > 10000) {
    lastTimeDevice = now;
    // strcpy(device.buffer, "timeout: 10s");
    // onSend(1);
  }
  if (tasks[KEY_DEVICE]) {
    onSend(device.direction);
    tasks[KEY_DEVICE] = 0;
  };
}