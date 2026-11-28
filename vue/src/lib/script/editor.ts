import type { ISuggestion } from 'vuesp-components/types';

import { getEventArgNames } from './types.ts';
import type { CapabilityManifest, CompileError } from './types.ts';

export type CompletionKind =
  | 'keyword'
  | 'builtin'
  | 'function'
  | 'event'
  | 'slot'
  | 'data'
  | 'script';

export interface CompletionItem {
  readonly label: string;
  readonly detail: string;
  readonly kind: CompletionKind;
  readonly insert: string;
  readonly caret?: number;
}

export interface ISuggestionInsert {
  readonly insert: string;
  readonly caret?: number;
}

export interface CompletionResult {
  readonly items: readonly CompletionItem[];
  readonly start: number;
}

export interface ErrorRange {
  readonly start: number;
  readonly end: number;
  readonly message: string;
  readonly line: number;
  readonly column: number;
}

function errorOffset(source: string, error: CompileError): number {
  const offset = error.pos.offset;
  if (Number.isInteger(offset) && offset >= 0 && offset <= source.length) return offset;
  let line = 1;
  let cursor = 0;
  while (line < error.pos.line && cursor < source.length) {
    const next = source.indexOf('\n', cursor);
    if (next < 0) break;
    cursor = next + 1;
    line++;
  }
  return Math.min(source.length, cursor + Math.max(0, error.pos.column - 1));
}

function errorTokenEnd(source: string, start: number): number {
  if (start >= source.length) return source.length;
  const ch = source[start];
  if (ch === "'" || ch === '"') {
    let cursor = start + 1;
    while (cursor < source.length) {
      if (source[cursor] === '\\') cursor += 2;
      else if (source[cursor] === ch) return cursor + 1;
      else cursor++;
    }
    return Math.min(source.length, start + 1);
  }
  if (/[A-Za-z0-9_$]/.test(ch)) {
    let cursor = start + 1;
    while (cursor < source.length && /[A-Za-z0-9_$]/.test(source[cursor])) cursor++;
    return cursor;
  }
  return Math.min(source.length, start + 1);
}

export function computeErrorRanges(
  source: string,
  errors: readonly CompileError[],
): ErrorRange[] {
  const ranges: ErrorRange[] = [];
  for (const error of errors) {
    let start = errorOffset(source, error);
    while (start < source.length && source[start] !== '\n' && /\s/.test(source[start])) start++;
    if (start >= source.length || source[start] === '\n') {
      if (source.length === 0) continue;
      start = source.length - 1;
    }
    const end = Math.max(start + 1, errorTokenEnd(source, start));
    ranges.push({
      start,
      end: Math.min(source.length, end),
      message: error.message,
      line: error.pos.line,
      column: error.pos.column,
    });
  }

  ranges.sort((a, b) => a.start - b.start || a.end - b.end);
  const merged: ErrorRange[] = [];
  for (const range of ranges) {
    const previous = merged[merged.length - 1];
    if (previous && range.start <= previous.end) {
      merged[merged.length - 1] = {
        ...previous,
        end: Math.max(previous.end, range.end),
        message: previous.message.includes(range.message)
          ? previous.message
          : `${previous.message}\n${range.message}`,
      };
      continue;
    }
    merged.push(range);
  }
  return merged;
}

const KEYWORDS = ['if', 'else', 'while', 'break', 'continue', 'return', 'on', 'true', 'false'];

const BUILTINS: ReadonlyArray<{ name: string; insert: string; caret?: number; detail: string }> = [
  { name: 'log', insert: 'log()', caret: 4, detail: 'builtin' },
  { name: 'wait', insert: 'wait()', caret: 5, detail: 'builtin' },
  { name: 'call', insert: "call('')", caret: 6, detail: 'builtin' },
  { name: 'set', insert: 'set()', caret: 4, detail: 'builtin' },
  { name: 'get', insert: 'get()', caret: 4, detail: 'builtin' },
  { name: 'len', insert: 'len()', caret: 4, detail: 'builtin' },
  { name: 'chr', insert: 'chr()', caret: 4, detail: 'builtin' },
  { name: 'ord', insert: 'ord()', caret: 4, detail: 'builtin' },
];

const TOKEN_RE =
  /\/\/[^\n]*|\/\*[\s\S]*?\*\/|'(?:\\.|[^'\\])*'|"(?:\\.|[^"\\])*"|\b\d+(?:\.\d+)?\b|\$[A-Za-z_][A-Za-z0-9_]*|\b[A-Za-z_][A-Za-z0-9_]*\b|[{}()[\];:,]|[+\-*/%<>=!&|?:.]+/g;

const TOKEN_CLASS: Record<string, string> = {
  comment: 'text-gray-dark-25 dark:text-gray-main',
  string: 'text-amber-dark-75 dark:text-amber-dark-35',
  number: 'text-green-dark-55 dark:text-green-light-25',
  keyword: 'text-purple-main dark:text-purple-light-30',
  variable: 'text-cyan-dark-30 dark:text-cyan-light-40',
  data: 'text-cyan-dark-30 dark:text-cyan-light-40',
  function: 'text-blue-600 dark:text-blue-400',
  event: 'text-amber-dark-75 dark:text-amber-light-45',
  operator: 'text-amber-dark-40',
  punctuation: 'text-gray-dark-30 dark:text-gray-light-10',
};

function escapeHtml(value: string): string {
  return value.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;');
}

const ERROR_CLASS =
  'rounded-xs bg-red-main/10 underline decoration-red-main decoration-wavy decoration-2 underline-offset-2';

function wrapText(text: string, kind: string, error = false): string {
  if (!text) return '';
  const cls = [TOKEN_CLASS[kind] ?? '', error ? ERROR_CLASS : '']
    .filter(Boolean)
    .join(' ');
  const content = escapeHtml(text);
  return cls ? `<span class="${cls}">${content}</span>` : content;
}

function renderText(
  text: string,
  kind: string,
  baseOffset: number,
  ranges: readonly ErrorRange[],
): string {
  if (!text) return '';
  if (ranges.length === 0) return wrapText(text, kind);

  let html = '';
  let cursor = baseOffset;
  const end = baseOffset + text.length;

  for (const range of ranges) {
    if (range.end <= cursor) continue;
    if (range.start >= end) break;
    const start = Math.max(cursor, range.start);
    const stop = Math.min(end, range.end);
    if (start > cursor) {
      html += wrapText(text.slice(cursor - baseOffset, start - baseOffset), kind);
    }
    if (stop > start) {
      html += wrapText(text.slice(start - baseOffset, stop - baseOffset), kind, true);
    }
    cursor = stop;
  }

  if (cursor < end) {
    html += wrapText(text.slice(cursor - baseOffset), kind);
  }
  return html;
}

export function highlightBytecode(
  source: string,
  manifest: CapabilityManifest,
  errorRanges: readonly ErrorRange[] = [],
  suggestions: readonly ISuggestion[] = [],
): string {
  const functions = new Set(manifest.functions.map((fn) => fn.name));
  const events = new Set(manifest.events.map((event) => event.name));
  for (const suggestion of suggestions) {
    if (suggestion.type === 'func') functions.add(suggestion.label);
    if (suggestion.type === 'event') events.add(suggestion.label);
  }
  const dataVars = new Set((manifest.dataVars ?? []).map((item) => item.name));
  const locals = new Set<string>();
  const handlerRe = /on\s*\(\s*(['"])[^'"]*\1\s*,\s*\(([^)]*)\)\s*=>/g;
  for (const match of source.matchAll(handlerRe)) {
    for (const name of match[2].split(',')) {
      const trimmed = name.trim();
      if (trimmed) locals.add(trimmed);
    }
  }
  const keywords = new Set(KEYWORDS);

  const re = new RegExp(TOKEN_RE.source, 'g');
  let html = '';
  let last = 0;
  for (const match of source.matchAll(re)) {
    const text = match[0];
    const index = match.index ?? 0;
    html += renderText(source.slice(last, index), '', last, errorRanges);

    let kind = '';
    const first = text[0];
    if (first === '/' && (text.startsWith('//') || text.startsWith('/*'))) {
      kind = 'comment';
    } else if (first === "'" || first === '"') {
      kind = 'string';
    } else if (first >= '0' && first <= '9') {
      kind = 'number';
    } else if (first === '$') {
      kind = dataVars.has(text) ? 'data' : 'variable';
    } else if (first === '_' || (first >= 'a' && first <= 'z') || (first >= 'A' && first <= 'Z')) {
      if (locals.has(text)) kind = 'variable';
      else if (functions.has(text) || BUILTINS.some((item) => item.name === text)) kind = 'function';
      else if (keywords.has(text)) kind = 'keyword';
      else if (events.has(text)) kind = 'event';
    } else if ('{}()[];:,'.includes(first)) {
      kind = 'punctuation';
    } else if ('+-*/%<>=!&|?:.'.includes(first)) {
      kind = 'operator';
    }

    html += renderText(text, kind, index, errorRanges);
    last = index + text.length;
  }
  html += renderText(source.slice(last), '', last, errorRanges);
  return `${html}\n`;
}

function slotItems(manifest: CapabilityManifest): CompletionItem[] {
  const items: CompletionItem[] = [];
  const add = (type: string, count: number, detail: string) => {
    for (let i = 0; i < count; i++) {
      const label = `$${type}${i}`;
      items.push({ label, detail, kind: 'slot', insert: label });
    }
  };
  add('u', manifest.vars.u, 'uint');
  add('i', manifest.vars.i, 'int');
  add('f', manifest.vars.f, 'float');
  add('s', manifest.vars.s, 'string');
  add('a', manifest.vars.a, 'array');
  add('p', manifest.vars.p, 'port');
  add('e', manifest.vars.e, 'event');
  add('c', manifest.vars.c, 'call');
  return items;
}

function eventSnippet(name: string, params: readonly string[] = []): { insert: string; caret: number } {
  if (params.length === 0) {
    const insert = `on('${name}') {\n  \n}\n`;
    return { insert, caret: insert.indexOf('\n') + 3 };
  }
  const insert = `on('${name}', (${params.join(', ')}) => {\n  \n});\n`;
  return { insert, caret: insert.indexOf('\n') + 3 };
}

function handlerParamsAt(source: string, caret: number): readonly string[] {
  const re = /on\s*\(\s*(['"])[^'"]*\1\s*,\s*\(([^)]*)\)\s*=>\s*\{/g;
  let last: RegExpExecArray | null = null;
  for (let match = re.exec(source); match; match = re.exec(source)) {
    if (match.index >= caret) break;
    last = match;
  }
  if (!last) return [];
  const open = last.index + last[0].length;
  let depth = 1;
  for (let i = open; i < caret; i++) {
    if (source[i] === '{') depth++;
    else if (source[i] === '}') {
      depth--;
      if (depth === 0) return [];
    }
  }
  return last[2]
    .split(',')
    .map((item) => item.trim())
    .filter(Boolean);
}

function keywordSnippet(label: string): { insert: string; caret?: number } {
  switch (label) {
    case 'if':
      return { insert: 'if () {\n  \n}\n', caret: 4 };
    case 'while':
      return { insert: 'while () {\n  \n}\n', caret: 7 };
    case 'else':
      return { insert: 'else {\n  \n}\n', caret: 9 };
    case 'on': {
      const snippet = eventSnippet('event');
      return { insert: snippet.insert, caret: 4 };
    }
    default:
      return { insert: label };
  }
}

function sortAndLimit(items: CompletionItem[], prefix: string): CompletionItem[] {
  const lower = prefix.toLowerCase();
  const seen = new Set<string>();
  const unique = items.filter((item) => {
    const key = `${item.kind}:${item.label}`;
    if (seen.has(key)) return false;
    seen.add(key);
    return true;
  });
  unique.sort((a, b) => {
    const aStarts = a.label.toLowerCase().startsWith(lower) ? 0 : 1;
    const bStarts = b.label.toLowerCase().startsWith(lower) ? 0 : 1;
    if (aStarts !== bStarts) return aStarts - bStarts;
    return a.label.localeCompare(b.label);
  });
  return unique.slice(0, 20);
}

function mergeSuggestionItems(items: CompletionItem[], suggestions: readonly ISuggestion[]): void {
  for (const suggestion of suggestions) {
    const detail = suggestion.detail || suggestion.description || suggestion.type;
    if (suggestion.type === 'func') {
      const index = items.findIndex((item) => item.label === suggestion.label);
      if (index >= 0) {
        items[index] = { ...items[index], detail };
        continue;
      }
      const { insert, caret } = normalizeSuggestionInsert(suggestion);
      items.push({ label: suggestion.label, detail, kind: 'function', insert, caret });
      continue;
    }
    if (suggestion.type === 'event') {
      const label = `on('${suggestion.label}')`;
      const index = items.findIndex((item) => item.label === label);
      if (index >= 0) {
        items[index] = { ...items[index], detail };
        continue;
      }
      const snippet = eventSnippet(suggestion.label, getEventArgNames(suggestion.label));
      items.push({ label, detail, kind: 'event', insert: snippet.insert, caret: snippet.caret });
      continue;
    }
    if (items.some((item) => item.label === suggestion.label)) continue;
    const { insert, caret } = normalizeSuggestionInsert(suggestion);
    items.push({
      label: suggestion.label,
      detail,
      kind: suggestion.type === 'var' ? 'slot' : 'keyword',
      insert,
      caret,
    });
  }
}

export function normalizeSuggestionInsert(suggestion: ISuggestion): ISuggestionInsert {
  const raw = suggestion.insert ?? suggestion.label;
  const marker = /\$\{[^}]*\}/g;
  let text = '';
  let last = 0;
  let first = -1;
  for (const match of raw.matchAll(marker)) {
    const index = match.index ?? 0;
    if (first < 0) first = text.length + (index - last);
    text += raw.slice(last, index);
    last = index + match[0].length;
  }
  text += raw.slice(last);
  const caret = first >= 0 ? first : (suggestion.cursorOffset ?? text.length);
  return { insert: text, caret };
}

export function getCompletions(
  source: string,
  caret: number,
  manifest: CapabilityManifest,
  scriptNames: readonly string[],
  force = false,
  suggestions: readonly ISuggestion[] = [],
): CompletionResult | null {
  const before = source.slice(0, caret);

  const callMatch = before.match(/call\(\s*(['"])([a-zA-Z0-9_-]*)$/);
  if (callMatch) {
    const prefix = callMatch[2];
    const start = caret - prefix.length;
    const items = scriptNames
      .filter((name) => name.toLowerCase().startsWith(prefix.toLowerCase()))
      .map<CompletionItem>((name) => ({
        label: name,
        detail: 'script',
        kind: 'script',
        insert: name,
      }));
    const sorted = sortAndLimit(items, prefix);
    return sorted.length > 0 ? { items: sorted, start } : null;
  }

  const onMatch = before.match(/on\(\s*(['"])([a-zA-Z0-9_]*)$/);
  if (onMatch) {
    const prefix = onMatch[2];
    const start = caret - prefix.length;
    const items = manifest.events
      .filter((event) => event.name.toLowerCase().startsWith(prefix.toLowerCase()))
      .map<CompletionItem>((event) => ({
        label: event.name,
        detail: `${event.params} params`,
        kind: 'event',
        insert: event.name,
      }));
    for (const suggestion of suggestions) {
      if (suggestion.type !== 'event') continue;
      const detail = suggestion.detail || suggestion.description || 'event';
      const index = items.findIndex((item) => item.label === suggestion.label);
      if (index >= 0) {
        items[index] = { ...items[index], detail };
        continue;
      }
      items.push({
        label: suggestion.label,
        detail,
        kind: 'event',
        insert: suggestion.label,
      });
    }
    const sorted = sortAndLimit(items, prefix);
    return sorted.length > 0 ? { items: sorted, start } : null;
  }

  let prefix = '';
  const dollarMatch = before.match(/(\$[A-Za-z0-9_]*)$/);
  if (dollarMatch) {
    prefix = dollarMatch[1];
  } else {
    const wordMatch = before.match(/([A-Za-z_][A-Za-z0-9_]*)$/);
    if (wordMatch) prefix = wordMatch[1];
  }
  if (!prefix && !force) return null;

  const start = caret - prefix.length;
  const items: CompletionItem[] = [];

  if (prefix.startsWith('$')) {
    items.push(...slotItems(manifest));
    for (const dataVar of manifest.dataVars ?? []) {
      items.push({
        label: dataVar.name,
        detail: dataVar.kind,
        kind: 'data',
        insert: dataVar.name,
      });
    }
  } else {
    for (const name of handlerParamsAt(source, caret)) {
      items.push({
        label: name,
        detail: 'event argument',
        kind: 'slot',
        insert: name,
      });
    }
    for (const keyword of KEYWORDS) {
      const snippet = keywordSnippet(keyword);
      items.push({
        label: keyword,
        detail: 'keyword',
        kind: 'keyword',
        insert: snippet.insert,
        caret: snippet.caret,
      });
    }
    for (const builtin of BUILTINS) {
      items.push({
        label: builtin.name,
        detail: builtin.detail,
        kind: 'builtin',
        insert: builtin.insert,
        caret: builtin.caret,
      });
    }
    for (const fn of manifest.functions) {
      items.push({
        label: fn.name,
        detail: `${fn.min === fn.max ? fn.min : `${fn.min}..${fn.max}`} args -> ${fn.ret}`,
        kind: 'function',
        insert: `${fn.name}()`,
        caret: fn.name.length + 1,
      });
    }
    for (const event of manifest.events) {
      const snippet = eventSnippet(event.name, getEventArgNames(event.name));
      items.push({
        label: `on('${event.name}')`,
        detail: `${event.params} params`,
        kind: 'event',
        insert: snippet.insert,
        caret: snippet.caret,
      });
    }
  }

  mergeSuggestionItems(items, suggestions);

  const filtered = items.filter((item) => {
    if (!prefix) return true;
    return item.label.toLowerCase().startsWith(prefix.toLowerCase()) ||
      item.label.toLowerCase().includes(prefix.toLowerCase());
  });
  const sorted = sortAndLimit(filtered, prefix);
  return sorted.length > 0 ? { items: sorted, start } : null;
}

// Kept for potential future use in tests or docs.
export const EDITOR_KEYWORDS = [...KEYWORDS];
export const EDITOR_BUILTINS = BUILTINS.map((item) => item.name);
