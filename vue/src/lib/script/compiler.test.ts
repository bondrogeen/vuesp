import test from 'node:test';
import assert from 'node:assert/strict';

import { compile } from './compiler.ts';
import { decompile } from './decompiler.ts';
import { readContainer } from './container.ts';
import { BYTECODE_EXAMPLES } from './examples.ts';
import { DEFAULT_MANIFEST } from './types.ts';
import type { CompileError, CompileResult } from './types.ts';

function errorsText(errors: readonly CompileError[]): string {
  return errors.map((err) => `${err.code}@${err.pos.line}:${err.pos.column} ${err.message}`).join('\n');
}

function ok(result: CompileResult): Uint8Array {
  assert.ok(result.ok, errorsText(result.errors));
  assert.ok(result.bytecode, 'bytecode is missing');
  return result.bytecode;
}

function roundTrip(source: string): { first: CompileResult; source: string; second: CompileResult } {
  const first = compile(source);
  const bytecode = ok(first);
  const decoded = decompile(bytecode, { manifest: DEFAULT_MANIFEST });
  assert.ok(decoded.ok, errorsText(decoded.errors));
  assert.ok(decoded.source !== undefined);
  const second = compile(decoded.source);
  const secondBytes = ok(second);
  assert.deepEqual(Array.from(secondBytes), Array.from(bytecode));
  return { first, source: decoded.source, second };
}

test('simple assignment round-trips', () => {
  const result = roundTrip('$u0 = 5;');
  assert.equal(result.source, '$u0 = 5;\n');
});

test('decompile keeps one line per top-level statement', () => {
  const result = compile('$u0 = 1; $u1 = 2;');
  const decoded = decompile(ok(result), { manifest: DEFAULT_MANIFEST });
  assert.ok(decoded.ok, errorsText(decoded.errors));
  assert.equal(decoded.source, '$u0 = 1;\n$u1 = 2;\n');
});

test('operator precedence round-trips', () => {
  const result = roundTrip('$u0 = 1 + 2 * 3;');
  assert.equal(result.source, '$u0 = 1 + 2 * 3;\n');
});

test('parentheses are preserved by precedence', () => {
  const result = roundTrip('$u0 = (1 + 2) * 3;');
  assert.equal(result.source, '$u0 = (1 + 2) * 3;\n');
});

test('if/else round-trips', () => {
  const source = `$u0 = 1;
if ($u0 > 3) {
  $p2 = 255;
} else {
  $p2 = 0;
}`;
  const result = roundTrip(source);
  assert.match(result.source, /if \(\$u0 > 3\)/);
  assert.match(result.source, /else/);
});

test('while, break and continue round-trip', () => {
  const source = `$u0 = 0;
while ($u0 < 5) {
  $u0 = $u0 + 1;
  if ($u0 == 3) {
    continue;
  }
  log($u0);
}`;
  const result = roundTrip(source);
  assert.match(result.source, /while \(\$u0 < 5\)/);
  assert.match(result.source, /continue;/);
});

test('ternary round-trips', () => {
  const result = roundTrip('$u0 = $u1 > 0 ? 10 : 20;');
  assert.match(result.source, /\? 10 : 20/);
});

test('event handler round-trips', () => {
  const source = `on('btn') {
  log($e0, $e1);
  $p2 = 1;
}`;
  const result = roundTrip(source);
  assert.match(result.source, /on\('btn'\)/);
  assert.match(result.source, /\$e0/);
});

test('wait normalizes to milliseconds', () => {
  const result = roundTrip('wait(1s);');
  assert.equal(result.source, 'wait(1000ms);\n');
});

test('external function round-trips', () => {
  const result = roundTrip('$u0 = fade(2, 255, 1000);');
  assert.match(result.source, /fade\(2, 255, 1000\)/);
});

test('array literal and indexing round-trip', () => {
  const result = roundTrip('$a0 = [1, 2, 3]; $u0 = $a0[1];');
  assert.match(result.source, /\[1, 2, 3\]/);
  assert.match(result.source, /\$a0\[1\]/);
});

test('data provider assignment round-trips', () => {
  const result = roundTrip("$display = 'hello';");
  assert.match(result.source, /\$display = 'hello';/);
});

test('comments are ignored', () => {
  const result = roundTrip('// note\n$u0 = 1; /* inline */');
  assert.equal(result.source, '$u0 = 1;\n');
});

test('compile is deterministic', () => {
  const source = '$u0 = 1 + 2 * 3; log($u0);';
  const a = ok(compile(source));
  const b = ok(compile(source));
  assert.deepEqual(Array.from(a), Array.from(b));
});

test('container header and CRC are valid', () => {
  const bytes = ok(compile('$u0 = 1;'));
  const container = readContainer(bytes);
  assert.equal(container.header.magic, 'VBC1');
  assert.equal(container.header.version, 1);
  assert.equal(container.header.codeLength, container.code.length);
  assert.ok(container.header.constCount >= 0);

  const corrupted = new Uint8Array(bytes);
  corrupted[corrupted.length - 1] ^= 0xff;
  assert.throws(() => readContainer(corrupted), /CRC/);
});

test('unknown function is a compile error', () => {
  const result = compile('$u0 = nope(1);');
  assert.equal(result.ok, false);
  assert.equal(result.errors[0].code, 'E_UNKNOWN_FUNCTION');
});

test('wrong arity is a compile error', () => {
  const result = compile('fade(1, 2);');
  assert.equal(result.ok, false);
  assert.equal(result.errors[0].code, 'E_ARITY');
});

test('unknown event is a compile error', () => {
  const result = compile("on('nope') { }");
  assert.equal(result.ok, false);
  assert.equal(result.errors[0].code, 'E_UNKNOWN_EVENT');
});

test('read-only event slot is rejected', () => {
  const result = compile('$e0 = 1;');
  assert.equal(result.ok, false);
  assert.equal(result.errors[0].code, 'E_READONLY');
});

test('slot range is checked', () => {
  const result = compile('$u10 = 1;');
  assert.equal(result.ok, false);
  assert.equal(result.errors[0].code, 'E_SLOT_RANGE');
});

test('wide port slots round-trip', () => {
  const result = roundTrip('$p17 = 255; $u0 = $p17; $p39 = 1;');
  assert.match(result.source, /\$p17 = 255;/);
  assert.match(result.source, /\$u0 = \$p17;/);
  assert.match(result.source, /\$p39 = 1;/);
});

test('port above 39 is rejected', () => {
  const result = compile('$p40 = 1;');
  assert.equal(result.ok, false);
  assert.equal(result.errors[0].code, 'E_SLOT_RANGE');
});

test('duplicate handler is rejected', () => {
  const result = compile("on('btn') { } on('btn') { }");
  assert.equal(result.ok, false);
  assert.equal(result.errors[0].code, 'E_DUPLICATE_EVENT');
});

test('return outside handler is rejected', () => {
  const result = compile('return;');
  assert.equal(result.ok, false);
  assert.equal(result.errors[0].code, 'E_RETURN_SCOPE');
});

test('break outside loop is rejected', () => {
  const result = compile('break;');
  assert.equal(result.ok, false);
  assert.equal(result.errors[0].code, 'E_LOOP_SCOPE');
});

test('syntax error has a position', () => {
  const result = compile('if ($u0 > 1) { $u0 = 1;');
  assert.equal(result.ok, false);
  assert.ok(result.errors[0].code);
  assert.ok(result.errors[0].pos.line >= 1);
});

test('explicit return stays in handler', () => {
  const source = `on('btn') {
  if ($e1 == 1) {
    return;
  }
  $p2 = 0;
}`;
  const result = roundTrip(source);
  assert.match(result.source, /return;/);
});

test('studio examples compile', () => {
  for (const example of BYTECODE_EXAMPLES) {
    const result = compile(example.source);
    assert.ok(result.ok, `${example.name}: ${errorsText(result.errors)}`);
  }
});

test('call by name round-trips', () => {
  const result = roundTrip("call('blink', 1, 2);");
  assert.match(result.source, /call\('blink', 1, 2\);/);
});

test('call by name normalizes uppercase', () => {
  const result = compile("call('Blink');");
  assert.ok(result.ok, errorsText(result.errors));
  assert.ok(result.bytecode);
  const decoded = decompile(result.bytecode, { manifest: DEFAULT_MANIFEST });
  assert.ok(decoded.ok, errorsText(decoded.errors));
  assert.ok(decoded.source !== undefined);
  assert.match(decoded.source, /call\('blink'\);/);
});

test('call requires a string name', () => {
  const result = compile('call(1);');
  assert.equal(result.ok, false);
  assert.equal(result.errors[0].code, 'E_CALL_NAME');
});

test('call rejects invalid script names', () => {
  for (const source of ["call('bad name');", "call('1abc');", "call('abcdefghijklmnopq');"]) {
    const result = compile(source);
    assert.equal(result.ok, false);
    assert.equal(result.errors[0].code, 'E_CALL_NAME');
  }
});
