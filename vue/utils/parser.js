const fs = require('fs').promises;
var path = require('path');

const args = process.argv;
const inputFile = args[2];
const outputFile = args[3];
if (!inputFile) process.exit(1);

const pathInput = path.normalize(process.cwd() + '/' + inputFile);
const pathOutput = outputFile ? process.cwd() + '/' + outputFile : pathInput;

const getCharCodes = s => {
  let charCodeArr = [];

  for (let i = 0; i < s.length; i++) {
    let code = s.charCodeAt(i);
    charCodeArr.push(code);
  }
  return charCodeArr;
};

const parseStruct = data => {
  data = data.replace(/\r?\n/g, '');
  const getStruct = data.match(/struct(.*?)};/gi);
  const getEnum = data.match(/enum(.*?)};/gi);

  const structs = {};
  let keys;

  getEnum.forEach(data => {
    // const enumName = data.match(/[^enum\s](\w*)/)[0];
    const all = data.match(/\w*,/g);
    keys = all.map(i => i.replace(',', ''));
  });

  getStruct.forEach(struct => {
    const structName = struct.match(/[^struct\s](\w*)/)[0];
    structs[structName] = [];
    const args = struct.match(/{(.*)}/)[0].replace(/[{,}]/g, '');
    const variables = args.match(/[^\s*](\w*) (\w*|\w*\[\d*\]);/g);
    variables.forEach(variable => {
      const [type, value] = variable.split(' ');
      const name = value.match(/(\w*)/)[0];
      const length = value.match(/\d+/)?.[0];
      const res = { t: type, n: name };
      if (length) res.l = +length;
      structs[structName].push(res);
    });
  });

  return { keys, structs };
};

const toHexString = byteArray => Array.from(byteArray, byte => '0x' + ('0' + (byte & 0xff).toString(16)).slice(-2)).join(',');

(async () => {
  try {
    let data = await fs.readFile(pathInput, 'utf8');
    const result = getCharCodes(JSON.stringify(parseStruct(data)));
    // const text = `const uint8_t initData[] PROGMEM = { 0x00, ${toHexString([...result])} };`; // esp 8266 if PROGMEM < 1324 = (reboot)
    const text = `const uint8_t initData[] = { 0x00, ${toHexString([...result])} };`;
    await fs.writeFile(`${pathOutput}/struct.h`, text);
    console.log(`Convert struct: ${pathInput} to struct.h`);
    // console.log(text);
  } catch (error) {
    console.log(error);
  }
})();
