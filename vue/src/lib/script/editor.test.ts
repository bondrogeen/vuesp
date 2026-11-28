import test from 'node:test';
import assert from 'node:assert/strict';

import { DEFAULT_MANIFEST } from './types.ts';
import { computeErrorRanges, getCompletions, highlightBytecode, normalizeSuggestionInsert } from './editor.ts';
import type { ISuggestion } from 'vuesp-components/types';

const manifest = DEFAULT_MANIFEST;
const scripts = ['boot', 'blink', 'display', 'chain'];

function labels(input: string, caret = input.length, force = false): string[] {
  const result = getCompletions(input, caret, manifest, scripts, force);
  return result ? result.items.map((item) => item.label) : [];
}

test('highlight marks slots, functions and keywords', () => {
  const html = highlightBytecode("$u0 = fade(1, 2, 3);\nwhile ($u0 < 3) { break; }", manifest);
  assert.match(html, /\$u0/);
  assert.match(html, /text-cyan-dark-30/);
  assert.match(html, /text-blue-600/);
  assert.match(html, /text-purple-main/);
});

test('highlight marks comments and strings', () => {
  const html = highlightBytecode("// comment\n$s0 = 'ok';\n/* block */", manifest);
  assert.match(html, /text-gray-dark-25/);
  assert.match(html, /text-amber-dark-75/);
});

test('highlight escapes html', () => {
  const html = highlightBytecode("$s0 = '<script>';", manifest);
  assert.match(html, /&lt;script&gt;/);
  assert.doesNotMatch(html, /<script>/);
});

test('call context suggests stored scripts', () => {
  const result = getCompletions("call('di", 8, manifest, scripts);
  assert.ok(result);
  assert.deepEqual(result.items.map((item) => item.label), ['display']);
  assert.equal(result.start, 6);
});

test('call context without matching scripts returns null', () => {
  assert.equal(getCompletions("call('zz", 8, manifest, scripts), null);
});

test('on context suggests event names only', () => {
  const result = getCompletions("on('bt", 6, manifest, scripts);
  assert.ok(result);
  assert.ok(result.items.some((item) => item.label === 'btn'));
  assert.ok(result.items.every((item) => item.insert === item.label));
});

test('slot prefix suggests slots and data vars', () => {
  assert.ok(labels('$u').includes('$u0'));
  assert.ok(labels('$d').includes('$display'));
});

test('function prefix suggests manifest function', () => {
  const result = getCompletions('fad', 3, manifest, scripts);
  assert.ok(result);
  const fade = result.items.find((item) => item.label === 'fade');
  assert.ok(fade);
  assert.equal(fade.insert, 'fade()');
  assert.equal(fade.caret, 5);
});

test('builtin call inserts empty script name', () => {
  const result = getCompletions('cal', 3, manifest, scripts);
  assert.ok(result);
  const call = result.items.find((item) => item.label === 'call');
  assert.ok(call);
  assert.equal(call.insert, "call('')");
  assert.equal(call.caret, 6);
});

test('keyword while inserts block snippet', () => {
  const result = getCompletions('whi', 3, manifest, scripts);
  assert.ok(result);
  const item = result.items.find((entry) => entry.label === 'while');
  assert.ok(item);
  assert.equal(item.insert, 'while () {\n  \n}\n');
});

test('empty prefix only opens when forced', () => {
  assert.equal(getCompletions('', 0, manifest, scripts), null);
  assert.ok(labels('', 0, true).length > 0);
});

test('error ranges underline the token at the position', () => {
  const source = '$u0 = ;';
  const ranges = computeErrorRanges(source, [
    { code: 'E_SYNTAX', message: 'unexpected token', pos: { line: 1, column: 7, offset: 6 } },
  ]);
  assert.equal(ranges.length, 1);
  assert.equal(source.slice(ranges[0].start, ranges[0].end), ';');
  assert.equal(ranges[0].message, 'unexpected token');
});

test('error ranges underline a whole identifier and merge overlaps', () => {
  const source = '$u0 = missing;';
  const ranges = computeErrorRanges(source, [
    { code: 'E_A', message: 'first', pos: { line: 1, column: 7, offset: 6 } },
    { code: 'E_B', message: 'second', pos: { line: 1, column: 9, offset: 8 } },
  ]);
  assert.equal(ranges.length, 1);
  assert.equal(source.slice(ranges[0].start, ranges[0].end), 'missing');
  assert.match(ranges[0].message, /first/);
  assert.match(ranges[0].message, /second/);
});

test('highlight adds error decorations for diagnostics', () => {
  const source = '$u0 = ;';
  const ranges = computeErrorRanges(source, [
    { code: 'E_SYNTAX', message: 'unexpected token', pos: { line: 1, column: 7, offset: 6 } },
  ]);
  const html = highlightBytecode(source, manifest, ranges);
  assert.match(html, /decoration-red-main/);
  assert.match(html, />;</);
});

const deviceSuggestions: readonly ISuggestion[] = [
  { label: 'fade', type: 'func', detail: 'fade(gpio, value, time)' },
  { label: 'custom', type: 'func', insert: 'custom(${0}, ${1})' },
  { label: 'btn_c', type: 'event', description: 'click count' },
  { label: '$p', type: 'var', insert: '$p${0}' },
  { label: 'blink', type: 'word', insert: 'blink();' },
];

test('suggestions override manifest function details', () => {
  const result = getCompletions('fa', 2, manifest, [], false, deviceSuggestions);
  const fade = result?.items.find((item) => item.label === 'fade');
  assert.equal(fade?.detail, 'fade(gpio, value, time)');
});

test('suggestions add functions, words and variables', () => {
  const custom = getCompletions('cus', 3, manifest, [], false, deviceSuggestions);
  assert.ok(custom?.items.some((item) => item.label === 'custom' && item.kind === 'function'));
  const word = getCompletions('bli', 3, manifest, [], false, deviceSuggestions);
  assert.ok(word?.items.some((item) => item.label === 'blink' && item.kind === 'keyword'));
  const variable = getCompletions('$p', 2, manifest, [], true, deviceSuggestions);
  assert.ok(variable?.items.some((item) => item.label === '$p' && item.kind === 'slot'));
});

test('suggestions add events inside on()', () => {
  const result = getCompletions("on('bt", 6, manifest, [], false, deviceSuggestions);
  const event = result?.items.find((item) => item.label === 'btn_c');
  assert.ok(event);
  assert.equal(event?.detail, 'click count');
});

test('suggestion placeholders move the caret to the first marker', () => {
  const normalized = normalizeSuggestionInsert({ label: 'custom', type: 'func', insert: 'custom(${0}, ${1})' });
  assert.equal(normalized.insert, 'custom(, )');
  assert.equal(normalized.caret, 'custom('.length);
});

test('suggestions extend highlighting', () => {
  const html = highlightBytecode('custom(1);', manifest, [], deviceSuggestions);
  assert.match(html, /text-blue-600/);
});

test('handler arguments are highlighted as local variables', () => {
  const html = highlightBytecode("on('btn_c', (gpio, value, count) => {\n  log(gpio);\n});", manifest);
  assert.match(html, /text-cyan-dark-30/);
});

test('handler arguments are offered inside the callback', () => {
  const source = "on('btn_c', (gpio, value, count) => {\n  gp";
  const result = getCompletions(source, source.length, manifest, scripts);
  assert.ok(result);
  assert.ok(result.items.some((item) => item.label === 'gpio' && item.kind === 'slot'));
});

test('event completion inserts named handler arguments', () => {
  const result = getCompletions('o', 1, manifest, scripts);
  assert.ok(result);
  const item = result.items.find((entry) => entry.label === "on('btn_c')");
  assert.ok(item);
  assert.match(item.insert, /on\('btn_c', \(gpio, value, count\) =>/);
});

test('time event completion inserts hours and minutes', () => {
  const result = getCompletions('ti', 2, manifest, scripts);
  assert.ok(result);
  const item = result.items.find((entry) => entry.label === "on('time')");
  assert.ok(item);
  assert.match(item.insert, /on\('time', \(hours, minutes\) =>/);
});
