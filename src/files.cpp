#include "./include/files.h"

void initFS() {
  if (!LittleFS.begin() && LittleFS.format()) Serial.println(F("Filesystem formatted!"));
}

void getInfo(Info& infoFS, u_int32_t id) {
#if defined(ESP8266)
  FSInfo fs_info;
  LittleFS.info(fs_info);
  infoFS.totalBytes = fs_info.totalBytes;
  infoFS.usedBytes = fs_info.usedBytes;
#elif defined(ESP32)
  infoFS.totalBytes = LittleFS.totalBytes();
  infoFS.usedBytes = LittleFS.usedBytes();
#endif
  infoFS.id = id;
  return;
}

void listDir(const char* dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\r\n", dirname);

  File root = LittleFS.open(dirname);
  if (!root) {
    Serial.println("- failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println(" - not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");

      Serial.print(file.name());
      time_t t = file.getLastWrite();
      struct tm* tmstruct = localtime(&t);
      Serial.printf("  LAST WRITE: %d-%02d-%02d %02d:%02d:%02d\n", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);

      if (levels) {
        listDir(file.name(), levels - 1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");

      Serial.print(file.size());
      time_t t = file.getLastWrite();
      struct tm* tmstruct = localtime(&t);
      Serial.printf("  LAST WRITE: %d-%02d-%02d %02d:%02d:%02d\n", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);
    }
    file = root.openNextFile();
  }
}

uint8_t readFile(const char* path, uint8_t* buf, size_t size) {
  Serial.println(path);
  Serial.println("readFile");

  File file = LittleFS.open(path, "r");
  Serial.println(file);
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

// void writeFile(const char* path, const uint8_t* buf, size_t size) {
//   File file = LittleFS.open(path, FILE_WRITE);
//   file.write(buf, size);
//   delay(1);
//   file.close();
// }

bool createDir(const char* path) {
  Serial.printf("Creating Dir: %s\n", path);
  if (LittleFS.mkdir(path)) {
    Serial.println("Dir created");
    return 1;
  } else {
    Serial.println("mkdir failed");
    return 0;
  }
}

void writeFile(const char* path, const uint8_t* buf, size_t size) {
  if (!LittleFS.exists(path)) {
    if (strchr(path, '/')) {
      Serial.printf("Create missing folders of: %s\r\n", path);
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

  Serial.printf("Writing file to: %s\r\n", path);
  File file = LittleFS.open(path, "w");
  if (!file) {
    Serial.println("- failed to open file for writing");
    return;
  }
  file.write(buf, size);
  delay(1);
  file.close();
}
