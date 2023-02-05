#include "./include/tasks.h"
uint8_t tasks[END];
// uint32_t lastTime = 0;

Scan scan = {SCAN, 0, 0, 0, 0, 0, ""};
Files files = {FILES, 0, 0, 0, 0, ""};

void onWsEvent(void *arg, uint8_t *data, size_t len, uint32_t clientId) {
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len) {
    if (info->opcode == WS_BINARY) {
      uint8_t task = data[0];
      if (task > sizeof(tasks)) return;
      if (info->len == 1) {
        tasks[task] = true;
      } else {
        if (task == SETTINGS && info->len == sizeof(settings)) {
          memcpy(&settings, data, sizeof(settings));
          saveSettings(settings);
        }
        if (task == FILES && info->len == sizeof(files)) {
          memcpy(&files, data, sizeof(files));
          tasks[task] = true;
        }
      }
    }
  }
}

#if defined(ESP8266)
void getFile(char *name) {
  Dir dir = LittleFS.openDir(name);
  while (dir.next()) {
    memset(files.name, 0, sizeof(files.name));
    dir.fileName().toCharArray(files.name, 32);
    files.size = dir.fileSize();
    files.isFile = dir.isFile();
    files.isDir = dir.isDirectory();
    wsSend((uint8_t *)&files, sizeof(files));
  }
  tasks[FILES] = false;
}

#elif defined(ESP32)

void getFile(char *name) {
  File root = LittleFS.open(name);
  File file = root.openNextFile();
  String str = "";
  while (file) {
    memset(files.name, 0, sizeof(files.name));
    strcpy(files.name, file.name());
    files.size = file.size();
    files.isFile = !file.isDirectory();
    files.isDir = file.isDirectory();
    wsSend((uint8_t *)&files, sizeof(files));
    file = root.openNextFile();
  }
  tasks[FILES] = false;
}
#endif

void scanWiFi() {
  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; i++) {
    scan.id = i;
    memset(scan.ssid, 0, sizeof(scan.ssid));
    WiFi.SSID(i).toCharArray(scan.ssid, 32);
    scan.channel = WiFi.channel(i);
    scan.rssi = WiFi.RSSI(i);
    scan.encryptionType = WiFi.encryptionType(i);
    // scan.isHidden = WiFi.isHidden(i);
    wsSend((uint8_t *)&scan, sizeof(scan));
  };
  tasks[SCAN] = false;
}

void send(uint8_t *message, size_t len, uint8_t task) {
  wsSend(message, len);
  tasks[task] = false;
}

void loopTask(uint32_t now) {
  if (tasks[SETTINGS]) send((uint8_t *)&settings, sizeof(settings), SETTINGS);
  if (tasks[INFO]) send((uint8_t *)&infoFS, sizeof(infoFS), INFO);
  if (tasks[FILES]) getFile(files.name);
  if (tasks[REBOOT]) reboot();
  if (tasks[SCAN]) scanWiFi();
}