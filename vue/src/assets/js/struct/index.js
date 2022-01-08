import structs from './base'

class Struct {
  constructor() {
    this.keys = [
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
  }

  getCommand (command) {
    return this.keys.findIndex(i => i === command)
  }

  cleanString (data) {
    const obj = {}
    for (const key in data) {
      obj[key] =
        typeof data[key] === 'string' ? data[key].replace(/\0/g, '') : data[key]
    }
    return obj
  }

  get (data) {
    if (data instanceof ArrayBuffer) {
      const comm = this.keys[new Uint8Array(data)[0]]
      if (comm) {
        const struct = structs[comm]
        if (struct) {
          return this.cleanString(new struct(data))
        }
      }
    }
    console.warn(`No struct from arr: ${data}`)
    return null
  }

  set (comm, data) {
    if (comm) {
      const struct = structs[comm] || structs['COMMAND']
      const command = this.getCommand(comm)
      if (struct && command !== -1) {
        const bufferObject = new struct(new ArrayBuffer(struct.byteLength))
        if (bufferObject) {
          for (const key in bufferObject) {
            bufferObject[key] = data[key]
          }
          bufferObject['key'] = command
          return bufferObject
        }
      }
    }
    console.warn(`No struct or key ${comm}`)
    return null
  }
}

const struct = new Struct()
export { struct }