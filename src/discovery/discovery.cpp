#include "./discovery.h"

Discovery discovery = {
    KEY_DISCOVERY,
};
uint32_t lastTimeDiscovery = 0;
WiFiUDP udp;

bool udpSendPacket(const uint8_t* data, size_t length, uint32_t targetAddress) {
  IPAddress targetIp(targetAddress);
  udp.beginPacket(targetIp, settings.discoveryPort);
  udp.write(data, length);
  bool ok = udp.endPacket();
  Serial.printf("[SEND] %d bytes -> %s:%d\n", length, targetIp.toString().c_str(), settings.discoveryPort);
  return ok;
}

void udpReceive() {
  int packetSize = udp.parsePacket();
  while (packetSize > 0) {
    IPAddress senderIp = udp.remoteIP();
    uint8_t buffer[TX_BUFFER_SIZE];
    int len = udp.read(buffer, packetSize);
    Serial.printf("[RECV] %d bytes <- %s\n", len, senderIp.toString().c_str());
    protocol.handleIncomingData((uint32_t)senderIp, buffer, len);
    packetSize = udp.parsePacket();
  }
}

void setupDiscovery() {
  if (!settings.discovery) return;
  udp.begin(settings.discoveryPort);

  // protocol.setup(infoFS.id);

  // protocol.onSend([](const OutgoingPacket& p) {
  //   return udpSendPacket(p.data, p.length, p.targetAddress);
  // });

  // protocol.onPeerDiscovered([](const PeerInfo& peer) {
  //   discovery.id = peer.id;
  //   discovery.ip = peer.address;
  //   discovery.lastSeen = peer.lastSeen;
  //   discovery.status = 1;
  //   wsSendAll((uint8_t*)&discovery, sizeof(discovery));
  //   Serial.printf("+ PEER: 0x%08X @ %s\n", peer.id, IPAddress(peer.address).toString().c_str());
  // });

  // protocol.onPeerLost([](const PeerInfo& peer) {
  //   discovery.id = peer.id;
  //   discovery.ip = peer.address;
  //   discovery.lastSeen = peer.lastSeen;
  //   discovery.status = 0;
  //   wsSendAll((uint8_t*)&discovery, sizeof(discovery));
  //   Serial.printf("- PEER LOST: 0x%08X\n", peer.id);
  // });

  // protocol.onTextReceived([](uint32_t senderId, const String& text) {
  //   Serial.printf("TEXT from 0x%08X: %s\n", senderId, text.c_str());
  //   // scriptRunner.registerScript(2, text.c_str());
  //   // scriptRunner.runScript(2);
  // });

  // protocol.onBinaryReceived([](uint32_t senderId, const uint8_t* data, size_t len) {
  //   Serial.printf("BINARY from 0x%08X, %d bytes: ", senderId, len);
  //   for (size_t i = 0; i < len; i++) Serial.printf("%02X ", data[i]);
  //   Serial.println();
  // });
}

uint8_t v = 0;
void loopDiscovery(uint32_t now) {
  if (!settings.discovery) return;
  if (!isConnected) return;
  // udpReceive();
  // protocol.update();

  // if (now - lastTimeDiscovery > settings.discoveryInterval * 1000) {
  //   lastTimeDiscovery = now;
  //   protocol.sendAnnounce();
  // }

  // static unsigned long lastText = 0;
  // if (now - lastText > 10000) {
  //   lastText = now;
  //   v = v == 0 ? 1 : 0;
  //   protocol.sendText(0, v ? "$p13=255" : "$p13=0");
  // }

  // static unsigned long lastBin = 0;
  // if (now - lastBin > 15000) {
  //   lastBin = now;
  //   uint8_t cmd[] = {0x01, 0x02, 0x03};
  //   protocol.sendBinary(14291611, cmd, 3);
  // }
}
