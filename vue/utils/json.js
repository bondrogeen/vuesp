import { promises as fs } from 'fs';
import { join, normalize } from 'path';

const args = process.argv;
const dir = args[2];
const output = args[3];
if (!dir || !output) process.exit(1);

const pathDir = normalize(process.cwd() + '/' + dir);
const pathOutput = normalize(process.cwd() + '/' + output);

const mergeJsonFiles = async (dir) => {
    try {
        const files = await fs.readdir(dir);
        const jsonFiles = files.filter(file => file.endsWith('.json'));
        const mergedData = {};
        for (const file of jsonFiles) {
            const filePath = join(dir, file);
            const fileContent = await fs.readFile(filePath, 'utf8');
            const jsonData = JSON.parse(fileContent);
            const fileName = file.replace('.json', '');
            mergedData[fileName] = jsonData;
        }
        return mergedData;
    } catch (error) {
        console.error('Ошибка при объединении файлов:', error);
        throw error;
    }
}

(async () => {
    try {
        const data = await mergeJsonFiles(pathDir);
        const result = JSON.stringify(data);
        await fs.writeFile(`${pathOutput}/default.json`, result);
        console.log(`Save json: ${pathOutput} to default.json`);
    } catch (error) {
        console.log(error);
    }
})();
