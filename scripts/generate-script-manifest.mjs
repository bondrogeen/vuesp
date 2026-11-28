#!/usr/bin/env node
// Generates vue/src/generated/scriptManifest.ts from src/generated/scriptManifest.h.
//
// The C++ header is the single source of truth for script events, external
// functions and limits. This script compiles a tiny host program that includes
// the header, prints it as JSON, and converts it to TypeScript.
//
// Usage:
//   node scripts/generate-script-manifest.mjs
//   node scripts/generate-script-manifest.mjs --check
//
// The web UI is a superset of all firmware builds, so Modbus entries are always
// included here. At runtime /vm/capabilities remains authoritative for a device.
import { execFileSync } from 'node:child_process';
import { existsSync, mkdtempSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';

const root = resolve(dirname(fileURLToPath(import.meta.url)), '..');
const generator = join(root, 'scripts/script_manifest_gen.cpp');
const output = join(root, 'vue/src/generated/scriptManifest.ts');
const check = process.argv.includes('--check');
const tempDir = mkdtempSync(join(tmpdir(), 'vuesp-manifest-'));
const binary = join(tempDir, 'manifest-gen');

const quote = (value) =>
  `'${String(value)
    .replace(/\\/g, '\\\\')
    .replace(/'/g, "\\'")
    .replace(/\n/g, '\\n')
    .replace(/\r/g, '\\r')}'`;

try {
  execFileSync('g++', ['-std=c++11', '-DVUESP_MODBUS=1', '-o', binary, generator], {
    stdio: 'pipe',
  });
  const manifest = JSON.parse(execFileSync(binary, { encoding: 'utf8' }));

  const splitArgs = (argsText) =>
    String(argsText)
      .split(',')
      .map((item) => item.trim())
      .filter(Boolean);

  const events = manifest.events.map((event) => ({
    id: event.id,
    name: event.name,
    params: event.params,
    args: splitArgs(event.args),
    description: event.description,
  }));
  const functions = manifest.functions.map((fn) => ({
    id: fn.id,
    name: fn.name,
    min: fn.minArgs,
    max: fn.maxArgs,
    ret: ['int', 'uint', 'float', 'string', 'array', 'none'][fn.ret] ?? 'int',
    description: fn.description,
  }));

  const lines = [
    '// AUTO-GENERATED from vuesp.config.yaml — do not edit by hand.',
    '// Run: node scripts/generate-script-manifest.mjs',
    '',
    'export const SCRIPT_EVENTS = [',
  ];

  for (const event of events) {
    lines.push(
      `  { id: ${event.id}, name: ${quote(event.name)}, params: ${event.params}, ` +
        `args: [${event.args.map(quote).join(', ')}], description: ${quote(event.description)} },`,
    );
  }

  lines.push('] as const;', '', 'export const SCRIPT_EVENT_ARG_NAMES: Readonly<Record<string, readonly string[]>> = {');
  for (const event of events) {
    lines.push(`  ${event.name}: [${event.args.map(quote).join(', ')}],`);
  }

  lines.push('};', '', 'export const SCRIPT_FUNCTIONS = [');
  for (const fn of functions) {
    lines.push(
      `  { id: ${fn.id}, name: ${quote(fn.name)}, min: ${fn.min}, max: ${fn.max}, ` +
        `ret: ${quote(fn.ret)}, description: ${quote(fn.description)} },`,
    );
  }

  lines.push('] as const;', '', 'export const SCRIPT_LIMITS = {');
  for (const [key, value] of Object.entries(manifest.limits)) {
    if (key === 'vars') {
      lines.push(`  vars: { ${Object.entries(value).map(([k, v]) => `${k}: ${v}`).join(', ')} },`);
    } else {
      lines.push(`  ${key}: ${value},`);
    }
  }
  lines.push('} as const;', '');

  const content = lines.join('\n');

  if (check) {
    if (!existsSync(output) || readFileSync(output, 'utf8') !== content) {
      console.error(
        '[script-manifest] generated file is stale; run: node scripts/generate-script-manifest.mjs',
      );
      process.exit(1);
    }
    console.log('[script-manifest] up to date');
  } else {
    writeFileSync(output, content);
    console.log(`[script-manifest] wrote ${output.replace(`${root}/`, '')}`);
  }
} finally {
  rmSync(tempDir, { recursive: true, force: true });
}
