export default [
  {
    id: 'device.input.1',
    name: 'input.1',
    key: 'device.input',
    get: (input) => (input & 0b00000001 ? 1 : 0)
  },
  {
    id: 'device.input.2',
    name: 'input.2',
    key: 'device.input',
    get: (input) => (input & 0b00000010 ? 1 : 0)
  },
  {
    id: 'device.input.3',
    name: 'input.3',
    key: 'device.input',
    get: (input) => (input & 0b00000100 ? 1 : 0)
  },
  {
    id: 'device.input.4',
    name: 'input.4',
    key: 'device.input',
    get: (input) => (input & 0b00001000 ? 1 : 0)
  },
  {
    id: 'device.input.5',
    name: 'input.5',
    key: 'device.input',
    get: (input) => (input & 0b00010000 ? 1 : 0)
  },
  {
    id: 'device.input.6',
    name: 'input.6',
    key: 'device.input',
    get: (input) => (input & 0b00100000 ? 1 : 0)
  },
  {
    id: 'device.output.1',
    name: 'output.1',
    key: 'device.output',
    get: (output) => (output & 0b00000001 ? 1 : 0),
    set: (output, value) => !value ? (output &= ~0b00000001) : (output |= 0b00000001)
  },
  {
    id: 'device.output.2',
    name: 'output.2',
    key: 'device.output',
    get: (output) => (output & 0b00000010 ? 1 : 0),
    set: (output, value) => !value ? (output &= ~0b00000010) : (output |= 0b00000010)
  },
  {
    id: 'device.output.3',
    name: 'output.3',
    key: 'device.output',
    get: (output) => (output & 0b00000100 ? 1 : 0),
    set: (output, value) => !value ? (output &= ~0b00000100) : (output |= 0b00000100)
  },
  {
    id: 'device.output.4',
    name: 'output.4',
    key: 'device.output',
    get: (output) => (output & 0b00001000 ? 1 : 0),
    set: (output, value) => !value ? (output &= ~0b00001000) : (output |= 0b00001000)
  },
  {
    id: 'device.output.5',
    name: 'output.5',
    key: 'device.output',
    get: (output) => (output & 0b00010000 ? 1 : 0),
    set: (output, value) => !value ? (output &= ~0b00010000) : (output |= 0b00010000)
  },
  {
    id: 'device.output.6',
    name: 'output.6',
    key: 'device.output',
    get: (output) => (output & 0b00100000 ? 1 : 0),
    set: (output, value) => !value ? (output &= ~0b00100000) : (output |= 0b00100000)
  },
  {
    id: 'device.adc.1',
    name: 'adc.1',
    key: 'device.adc1',
    get: (adc1) => adc1 / 10,
  },
  {
    id: 'device.adc.2',
    name: 'adc.2',
    key: 'device.adc2',
    get: (adc2) => adc2,
  },
  {
    id: 'device.adc.3',
    name: 'adc.3',
    key: 'device.adc3',
    get: (adc3) => adc3,
  },
  {
    id: 'device.adc.4',
    name: 'adc.4',
    key: 'device.adc4',
    get: (adc4) => adc4,
  },
  {
    id: 'device.dac.1',
    name: 'dac.1',
    key: 'device.dac1',
    set: (output, value) => value,
    get: (dac1) => dac1,
  },
  {
    id: 'device.dac.2',
    name: 'dac.2',
    key: 'device.dac2',
    set: (output, value) => value,
    get: (dac2) => dac2,
  },
  {
    id: 'device.now',
    name: 'now',
    key: 'device.now',
    set: (output, value) => value,
    get: (now) => now,
  },
]
