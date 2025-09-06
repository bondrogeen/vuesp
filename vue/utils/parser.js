import { promises as fs } from 'fs';
import path from 'path'

import VuespStruct from 'vuesp-struct';

const parseStruct = VuespStruct.default.parseStruct

const args = process.argv;
const inputFile = args[2];
const outputFile = args[3];
if (!inputFile) process.exit(1);

const pathInput = path.normalize(process.cwd() + '/' + inputFile);
const pathOutput = outputFile ? process.cwd() + '/' + outputFile : pathInput;

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
