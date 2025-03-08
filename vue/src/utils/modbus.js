// var data;
// process.stdin.on('data', function (chunk) {
//     data = chunk
// });
// process.stdin.on('end', function () {
//     console.log(data);
//     var givenCrc = data.slice(data.length - 2);
//     givenCrc = (data[1] << 7 | data[0]);
//     console.log(givenCrc);
//     var calculatedCrc = CRC(data.slice(0, data.length - 2));
//     console.log(calculatedCrc);
// });
// process.stdin.resume();


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
    const b = x >> 8
    return (revers ? [a, b] : [b, a])
}