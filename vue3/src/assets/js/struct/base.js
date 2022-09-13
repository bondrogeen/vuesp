import Struct from './lib/index'

class Version extends Struct.extend(
  { name: 'one', type: 'Uint8' },
  { name: 'two', type: 'Uint8' },
  { name: 'three', type: 'Uint8' }
) {
  get () {
    return `${this.one}.${this.two}.${this.three}`
  }
  set (string) {
    var x = string.split('.')
    this.one = +x[0]
    this.two = +x[1]
    this.three = +x[2]
  }
}

class Ip extends Struct.extend(
  { name: 'one', type: 'Uint8' },
  { name: 'two', type: 'Uint8' },
  { name: 'three', type: 'Uint8' },
  { name: 'four', type: 'Uint8' }
) {
  get () {
    return `${this.one}.${this.two}.${this.three}.${this.four}`
  }
  set (string) {
    var x = string.split('.')
    this.one = +x[0]
    this.two = +x[1]
    this.three = +x[2]
    this.four = +x[3]
  }
}

const structs = {
  COMMAND: Struct.extend({
    name: 'key',
    type: Struct.types.Uint8
  }),

  PING: Struct.extend({
    name: 'key',
    type: Struct.types.Uint8
  }),

  SCAN: Struct.extend(
    { name: 'key', type: Struct.types.Uint8 },
    { name: 'id', type: Struct.types.Uint8 },
    { name: 'name', type: 'String', byteLength: 32 },
  ),

  FILES: Struct.extend(
    { name: 'key', type: Struct.types.Uint8 },
    { name: 'isDir', type: Struct.types.Uint8 },
    { name: 'isFile', type: Struct.types.Uint8 },
    { name: 'empty', type: Struct.types.Uint8 },
    { name: 'name', type: 'String', byteLength: 32 },
    { name: 'size', type: Struct.types.Uint32LE },
  ),

  PROGRESS: Struct.extend(
    { name: 'key', type: Struct.types.Uint8 },
    { name: 'empty', type: 'String', byteLength: 3 },
    { name: 'size', type: Struct.types.Uint32LE },
  ),

  INFO: Struct.extend(
    { name: 'key', type: Struct.types.Uint8 },
    { name: 'frimware', type: Version },
    { name: 'totalBytes', type: Struct.types.Uint32LE },
    { name: 'usedBytes', type: Struct.types.Uint32LE }
  ),

  SETTINGS: Struct.extend(
    { name: 'key', type: Struct.types.Uint8 },
    { name: 'version', type: Version },
    { name: 'serverUrl', type: 'String', byteLength: 32 },
    { name: 'serverPort', type: Struct.types.Uint16LE },

    { name: 'ethDhcp', type: Struct.types.Uint8 },
    { name: 'ethIp', type: Ip },
    { name: 'ethSubnet', type: Ip },
    { name: 'ethGeteway', type: Ip },
    { name: 'ethDns', type: Ip },

    { name: 'wifiDhcp', type: Struct.types.Uint8 },
    { name: 'wifiIp', type: Ip },
    { name: 'wifiSubnet', type: Ip },
    { name: 'wifiGeteway', type: Ip },
    { name: 'wifiDns', type: Ip },

    { name: 'wifiMode', type: Struct.types.Uint8 },
    { name: 'wifiSsid', type: 'String', byteLength: 32 },
    { name: 'wifiPass', type: 'String', byteLength: 32 },

    { name: 'httpMode', type: Struct.types.Uint8 },
    { name: 'httpLogin', type: 'String', byteLength: 12 },
    { name: 'httpPass', type: 'String', byteLength: 12 },

    { name: 'deviceName', type: 'String', byteLength: 12 }
  )

}

export default structs