
export const functionList = [
    { name: 'Read Coil Status 0x01', type: '', access: 'read', value: 0x01 },
    { name: 'Read Input Status 0x02', type: '', access: 'read', value: 0x02 },
    { name: 'Read Holding Registers 0x03', type: '', access: 'read', value: 0x03 },
    { name: 'Read Input Registers 0x04', type: '', access: 'read', value: 0x04 },
    { name: 'Force Single Coil 0x05', type: '', access: 'write', value: 0x05 },
    { name: 'Preset Single Register 0x06', type: '', access: 'write', value: 0x06 },
    { name: 'Force Multiple Coils 0x0f', type: '', access: 'write', value: 0x0f },
    { name: 'Preset Multiple Registers 0x10', type: '', access: 'write', value: 0x10 },
];

export const listBaudRate = [
    { name: '1200', value: 1200 },
    { name: '9600', value: 9600 },
    { name: '14400', value: 14400 },
    { name: '19220', value: 19220 },
    { name: '28800', value: 28800 },
    { name: '38400', value: 38400 },
    { name: '57600', value: 57600 },
    { name: '115200', value: 115200 },
];
export const listParity = [
    { name: 'None', value: 'None' },
    { name: 'Event', value: 'Event' },
    { name: 'Odd', value: 'Odd' },
];
export const listStopBits = [
    { name: '1', value: 1 },
    { name: '2', value: 2 },
];



var POLY = 0xA001;
var SEED = 0xFFFF;
const Calc_CRC = (b, crc) => {
    crc ^= b & 0xFF;
    for (var i = 0; i < 8; i++) {
        var carry = crc & 0x0001;
        crc >>= 1;
        if (carry) crc ^= POLY;
    }
    return crc;
}
export const CRC = (buffer) => {
    var crc = SEED;
    for (var i = 0; i < buffer.length; i++) {
        crc = Calc_CRC(buffer[i], crc);
    }
    return crc;
}

export const getInt16Bytes = (x, revers = false) => {
    const a = x & 255
    const b = (x >> 8) & 255
    return (revers ? [a, b] : [b, a])
}
export const getBytesInt16 = ([a, b]) => (((a & 0xFF) << 8) | (b & 0xFF));

export const intToHex = (d) => {
    const s = (+d).toString(16);
    return (s.length < 2) ? `0${s}` : s;
}