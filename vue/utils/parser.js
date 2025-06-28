import { promises as fs } from 'fs';
import path from 'path'

const args = process.argv;
const inputFile = args[2];
const outputFile = args[3];
if (!inputFile) process.exit(1);

const pathInput = path.normalize(process.cwd() + '/' + inputFile);
const pathOutput = outputFile ? process.cwd() + '/' + outputFile : pathInput;

const parseStruct = data => {
  data = data.replace(/\r?\n/g, '');
  const getStruct = data.match(/struct(.*?)};/gi);
  const getEnum = data.match(/enum(.*?)};/gi);

  const structs = {};
  let keys;

  getEnum.forEach(data => {
    const all = data.match(/\w*,/g);
    keys = all.map(i => i.replace(',', '').replace('KEY_', ''));
  });

  getStruct.forEach(struct => {
    const structName = struct.match(/[^struct\s](\w*)/)[0];
    structs[structName] = [];
    const args = struct.match(/{(.*)}/)[0].replace(/[{,}]/g, '');
    const variables = args.match(/[^\s*](\w*) (\w*|\w*\[\d*\]);/g);
    variables.forEach(variable => {
      const [type, value] = variable.split(' ');
      const name = value.match(/(\w*)/)[0];
      const length = value.match(/\[(\d+)\]/)?.[1];
      const res = { type, name };
      if (length) res.length = +length;
      structs[structName].push(res);
    });
  });

  return { keys, structs };
};

(async () => {
  try {
    let data = await fs.readFile(pathInput, 'utf8');
    const result = JSON.stringify(parseStruct(data));
    await fs.writeFile(`${pathOutput}/struct.json`, result);
    console.log(`Save struct: ${pathInput} to struct.json`);
  } catch (error) {
    console.log(error);
  }
})();
