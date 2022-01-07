const AppStruct = (() => {
  const command = [
    'OFF',
    'LOAD',
    'ERASE',
    'SETTING',
    'SAVE',
    'REBOOT',
    'INFO',
    'PING',
    'TEST'
  ]

  class Version extends Struct.extend(
    { name: 'one', type: 'Uint8' },
    { name: 'two', type: 'Uint8' },
    { name: 'three', type: 'Uint8' }
  ) {
    get() {
      return `${this.one}.${this.two}.${this.three}`
    }
    set(string) {
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
    get() {
      return `${this.one}.${this.two}.${this.three}.${this.four}`
    }
    set(string) {
      var x = string.split('.')
      this.one = +x[0]
      this.two = +x[1]
      this.three = +x[2]
      this.four = +x[3]
    }
  }

  const Command = Struct.extend({
    name: 'command',
    type: Struct.types.Uint8
  })

  const Ping = Struct.extend({
    name: 'init',
    type: Struct.types.Uint8
  })

  const Info = Struct.extend(
    { name: 'init', type: Struct.types.Uint8 },
    { name: 'frimware', type: Version },
    { name: 'totalBytes', type: Struct.types.Uint32LE },
    { name: 'usedBytes', type: Struct.types.Uint32LE }
  )

  const Settings = Struct.extend(
    { name: 'init', type: Struct.types.Uint8 },
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

  function transform(data) {
    const obj = {}
    for (const key in data) {
      obj[key] =
        typeof data[key] === 'string' ? data[key].replace(/\0/g, '') : data[key]
    }
    return obj
  }

  function newStruct(comm, data) {
    // console.log(comm);
    if (comm === command.indexOf('INFO')) {
      return new Info(data || new ArrayBuffer(Info.byteLength))
    } else if (comm === command.indexOf('SETTING')) {
      return new Settings(data || new ArrayBuffer(Settings.byteLength))
    } else if (comm === command.indexOf('PING')) {
      return new Ping(data || new ArrayBuffer(Ping.byteLength))
    } else {
      console.warn(`No struct ${comm}`)
      return null
    }
  }

  return {
    getStruct: function (data) {
      if (data instanceof ArrayBuffer) {
        return transform(newStruct(new Uint8Array(data)[0], data))
      }
    },
    getBuffer: function (data) {
      return newStruct(command.indexOf(data))
    },
    getCommand: function (data) {
      const buf = new Command(new ArrayBuffer(Command.byteLength))
      buf.command = command.indexOf(data)
      return buf
    },
    getAllCommand: function () {
      return command.reduce(function (result, item, index) {
        result[item] = index
        return result
      }, {})
    }
  }
})()
