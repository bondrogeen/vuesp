#!/usr/bin/env node
// Compile a JavaScript-like ScriptRunner source file to a VBC1 container.
// Usage: node scripts/compile-script.mjs <source.js> <output.vbc>
import fs from 'node:fs';
import { compile } from '../vue/src/lib/script/compiler.ts';
import { DEFAULT_MANIFEST } from '../vue/src/lib/script/types.ts';

const [input, output] = process.argv.slice(2);
if (!input || !output) {
  console.error('Usage: node scripts/compile-script.mjs <source.js> <output.vbc>');
  process.exit(1);
}

const source = fs.readFileSync(input, 'utf8');
const result = compile(source, { manifest: DEFAULT_MANIFEST });
if (!result.ok || !result.bytecode) {
  for (const err of result.errors) {
    console.error(`${err.pos.line}:${err.pos.column} ${err.code}: ${err.message}`);
  }
  process.exit(1);
}

fs.writeFileSync(output, result.bytecode);
console.log(`Wrote ${output} (${result.bytecode.length} bytes, stack ${result.stats?.stackMax ?? '?'})`);
