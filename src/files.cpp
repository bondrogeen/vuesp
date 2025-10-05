#include "./include/files.h"

void initFS() {
  if (!LittleFS.begin() && LittleFS.format()) Serial.println(F("Filesystem formatted!"));
}

void getInfo(Info* infoFS) {
#if defined(ESP8266)
  FSInfo fs_info;
  LittleFS.info(fs_info);
  infoFS->totalBytes = fs_info.totalBytes;
  infoFS->usedBytes = fs_info.usedBytes;
#elif defined(ESP32)
  infoFS->totalBytes = LittleFS.totalBytes();
  infoFS->usedBytes = LittleFS.usedBytes();
#endif
  return;
}

bool createDir(const char* path) {
  return LittleFS.mkdir(path);
}

uint8_t readFile(const char* path, uint8_t* buf, size_t size) {
  File file = LittleFS.open(path, "r");
  if (!file || file.isDirectory()) {
    return 0;
  } else {
    while (file.available()) {
      file.read(buf, size);
    }
    file.close();
    return 1;
  }
}

void writeFile(const char* path, const uint8_t* buf, size_t size) {
  if (!LittleFS.exists(path)) {
    if (strchr(path, '/')) {
      char* pathStr = strdup(path);
      if (pathStr) {
        char* ptr = strchr(pathStr, '/');
        while (ptr) {
          *ptr = 0;
          LittleFS.mkdir(pathStr);
          *ptr = '/';
          ptr = strchr(ptr + 1, '/');
        }
      }
      free(pathStr);
    }
  }

  File file = LittleFS.open(path, "w");
  if (!file) return;
  file.write(buf, size);
  delay(1);
  file.close();
}

void getLoadDef(const char* path, uint8_t* buf, size_t size) {
  uint8_t isOk = readFile(path, buf, size);
  // if (!isOk) {
  //   writeFile(path, buf, size);
  // }
}