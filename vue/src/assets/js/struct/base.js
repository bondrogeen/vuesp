import Struct from './lib/index';

class Version extends Struct.extend({ name: 'one', type: 'Uint8' }, { name: 'two', type: 'Uint8' }, { name: 'three', type: 'Uint8' }) {
  get() {
    return `${this.one}.${this.two}.${this.three}`;
  }
  set(string) {
    var [one, two, three] = string.split('.');
    this.one = +one;
    this.two = +two;
    this.three = +three;
  }
}

class Ip extends Struct.extend({ name: 'one', type: 'Uint8' }, { name: 'two', type: 'Uint8' }, { name: 'three', type: 'Uint8' }, { name: 'four', type: 'Uint8' }) {
  get() {
    return `${this.one}.${this.two}.${this.three}.${this.four}`;
  }
  set(string) {
    var [one, two, three, four] = string.split('.');
    this.one = +one;
    this.two = +two;
    this.three = +three;
    this.four = +four;
  }
}

const structs = {
  COMMAND: Struct.extend({ name: 'key', type: Struct.types.Uint8 }),
  PING: Struct.extend({ name: 'key', type: Struct.types.Uint8 }),

  SCAN: Struct.extend(
    { name: 'key', type: Struct.types.Uint8 },
    { name: 'id', type: Struct.types.Uint8 },
    { name: 'isHidden', type: Struct.types.Uint8 },
    { name: 'encryptionType', type: Struct.types.Uint8 },
    { name: 'channel', type: Struct.types.Int32LE },
    { name: 'rssi', type: Struct.types.Int32LE },
    { name: 'ssid', type: 'String', byteLength: 32 }
  ),

  FILES: Struct.extend(
    { name: 'key', type: Struct.types.Uint8 },
    { name: 'isDir', type: Struct.types.Uint8 },
    { name: 'isFile', type: Struct.types.Uint8 },
    { name: 'command', type: Struct.types.Uint8 },
    { name: 'size', type: Struct.types.Uint32LE },
    { name: 'name', type: 'String', byteLength: 32 }
  ),

  PROGRESS: Struct.extend(
    { name: 'key', type: Struct.types.Uint8 },
    { name: 'status', type: Struct.types.Uint8 },
    { name: 'empty', type: Struct.types.Uint16LE },
    { name: 'size', type: Struct.types.Uint32LE },
    { name: 'length', type: Struct.types.Uint32LE }
  ),

  INFO: Struct.extend(
    { name: 'key', type: Struct.types.Uint8 },
    { name: 'frimware', type: Version },
    { name: 'totalBytes', type: Struct.types.Uint32LE },
    { name: 'usedBytes', type: Struct.types.Uint32LE },
    { name: 'id', type: Struct.types.Uint32LE }
  ),

  SETTINGS: Struct.extend(
    { name: 'key', type: Struct.types.Uint8 },
    { name: 'wifiDhcp', type: Struct.types.Uint8 },
    { name: 'wifiMode', type: Struct.types.Uint8 },
    { name: 'authMode', type: Struct.types.Uint8 },

    { name: 'version', type: Struct.types.Uint16LE },
    { name: 'device', type: Struct.types.Uint16LE },

    { name: 'wifiIp', type: Ip },
    { name: 'wifiSubnet', type: Ip },
    { name: 'wifiGeteway', type: Ip },
    { name: 'wifiDns', type: Ip },

    { name: 'wifiSsid', type: 'String', byteLength: 32 },
    { name: 'wifiPass', type: 'String', byteLength: 32 },
    { name: 'authLogin', type: 'String', byteLength: 12 },
    { name: 'authPass', type: 'String', byteLength: 12 }
  ),
};

export default structs;
