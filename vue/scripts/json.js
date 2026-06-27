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
        const mergeDirectory = async (currentDir) => {
            const items = await fs.readdir(currentDir);
            const result = {};
            for (const item of items) {
                const itemPath = join(currentDir, item);
                const stat = await fs.stat(itemPath);
                if (stat.isDirectory()) {
                    result[item] = await mergeDirectory(itemPath);
                } else if (stat.isFile() && item.endsWith('.json')) {
                    const fileContent = await fs.readFile(itemPath, 'utf8');
                    const jsonData = JSON.parse(fileContent);
                    const fileName = item.replace('.json', '');
                    result[fileName] = jsonData;
                }
            }
            return result;
        };

        return await mergeDirectory(dir);
    } catch (error) {
        console.error('Ошибка при объединении файлов:', error);
        throw error;
    }
};

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
