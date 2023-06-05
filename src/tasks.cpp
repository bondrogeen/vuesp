#include "./include/tasks.h"
uint8_t tasks[KEY_END];
// uint32_t lastTime = 0;

Scan scan = {KEY_SCAN, 0, 0, 0, 0, 0, ""};
Files files = {KEY_FILES, 0, 0, 0, 0, ""};
Port port = {KEY_PORT, 0, 0, 0};

void onWsEventTasks(void *arg, uint8_t *data, size_t len, uint32_t clientId) {
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len) {
    if (info->opcode == WS_BINARY) {
      uint8_t task = data[0];
      if (task > sizeof(tasks)) return;
      if (info->len == 1) {
        tasks[task] = true;
      } else {
        if (task == KEY_SETTINGS && info->len == sizeof(settings)) {
          memcpy(&settings, data, sizeof(settings));
          saveSettings(settings);
        }
        if (task == KEY_FILES && info->len == sizeof(files)) {
          memcpy(&files, data, sizeof(files));
          tasks[task] = true;
        }
        if (task == KEY_PORT && info->len == sizeof(port)) {
          memcpy(&port, data, sizeof(port));
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
  tasks[KEY_FILES] = false;
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
  tasks[KEY_FILES] = false;
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
  tasks[KEY_SCAN] = false;
}

void send(uint8_t *message, size_t len, uint8_t task) {
  wsSend(message, len);
  tasks[task] = false;
}

void loopTask(uint32_t now) {
  if (tasks[KEY_SETTINGS]) send((uint8_t *)&settings, sizeof(settings), KEY_SETTINGS);
  if (tasks[KEY_INFO]) send((uint8_t *)&infoFS, sizeof(infoFS), KEY_INFO);
  if (tasks[KEY_FILES]) getFile(files.name);
  if (tasks[KEY_REBOOT]) reboot();
  if (tasks[KEY_SCAN]) scanWiFi();
}