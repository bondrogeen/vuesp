#include "./include/tasks.h"

#include "./include/eeprom.h"
#include "./include/files.h"

uint8_t tasks[KEY_END];

Scan scan = {KEY_SCAN, 0, 0, 0, 0, 0, ""};
Files files = {KEY_FILES, 0, 0, 0, 0, ""};
Port port = {KEY_PORT, 0, 0, 0};
Notification notification = {KEY_NOTIFICATION, 1, NOTIF_COLOR_TRANSPARENT, 0, 0, "Notification"};

void onWsEventTasks(void* arg, uint8_t* data, size_t len, uint32_t clientId, uint8_t task) {
  AwsFrameInfo* info = (AwsFrameInfo*)arg;
  tasks[task] = true;
  if (task == KEY_SETTINGS && info->len == sizeof(settings)) {
    memcpy(&settings, data, sizeof(settings));
    saveSettings(settings);
  }
  if (task == KEY_FILES && info->len == sizeof(files)) {
    memcpy(&files, data, sizeof(files));
  }
  if (task == KEY_PORT && info->len == sizeof(port)) {
    memcpy(&port, data, sizeof(port));
  }
  if (task == KEY_NOTIFICATION && info->len == sizeof(notification)) {
    memcpy(&notification, data, sizeof(notification));
  }
}

#if defined(ESP8266)
void getFile(char* name) {
  Dir dir = LittleFS.openDir(name);
  while (dir.next()) {
    memset(files.name, 0, sizeof(files.name));
    dir.fileName().toCharArray(files.name, 32);
    files.size = dir.fileSize();
    files.isFile = dir.isFile();
    files.isDir = dir.isDirectory();
    wsSend((uint8_t*)&files, sizeof(files));
  }
}

#elif defined(ESP32)

void getFile(char* name) {
  File root = LittleFS.open(name);
  File file = root.openNextFile();
  String str = "";
  while (file) {
    memset(files.name, 0, sizeof(files.name));
    strcpy(files.name, file.name());
    files.size = file.size();
    files.isFile = !file.isDirectory();
    files.isDir = file.isDirectory();
    wsSend((uint8_t*)&files, sizeof(files));
    file = root.openNextFile();
  }
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
    wsSend((uint8_t*)&scan, sizeof(scan));
  };
}

void sendNotification() {
  wsSendAll((uint8_t*)&notification, sizeof(notification));
}
void sendNotificationText(const char* message, uint8_t color) {
  notification.color = color;
  memset(notification.text, 0, sizeof(notification.text));
  strcpy(notification.text, message);
  sendNotification();
}

void loopTask(uint32_t now) {
  if (tasks[KEY_SETTINGS]) {
    wsSend((uint8_t*)&settings, sizeof(settings));
    tasks[KEY_SETTINGS] = false;
  };
  if (tasks[KEY_INFO]) {
    getInfo(&infoFS);
    wsSend((uint8_t*)&infoFS, sizeof(infoFS));
    tasks[KEY_INFO] = false;
  }
  if (tasks[KEY_FILES]) {
    getFile(files.name);
    tasks[KEY_FILES] = false;
  }
  if (tasks[KEY_REBOOT]) reboot();

  if (tasks[KEY_SCAN]) {
    scanWiFi();
    tasks[KEY_SCAN] = false;
  }
  if (tasks[KEY_NOTIFICATION]) {
    sendNotification();
    tasks[KEY_NOTIFICATION] = false;
  }
}