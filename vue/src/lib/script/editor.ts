import type { CapabilityManifest } from './types.ts';

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

export interface CompletionResult {
  readonly items: readonly CompletionItem[];
  readonly start: number;
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

export function highlightBytecode(source: string, manifest: CapabilityManifest): string {
  const functions = new Set(manifest.functions.map((fn) => fn.name));
  const events = new Set(manifest.events.map((event) => event.name));
  const dataVars = new Set((manifest.dataVars ?? []).map((item) => item.name));
  const keywords = new Set(KEYWORDS);

  const re = new RegExp(TOKEN_RE.source, 'g');
  let html = '';
  let last = 0;
  for (const match of source.matchAll(re)) {
    const text = match[0];
    const index = match.index ?? 0;
    html += escapeHtml(source.slice(last, index));

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
      if (functions.has(text) || BUILTINS.some((item) => item.name === text)) kind = 'function';
      else if (keywords.has(text)) kind = 'keyword';
      else if (events.has(text)) kind = 'event';
    } else if ('{}()[];:,'.includes(first)) {
      kind = 'punctuation';
    } else if ('+-*/%<>=!&|?:.'.includes(first)) {
      kind = 'operator';
    }

    const cls = kind ? TOKEN_CLASS[kind] : '';
    html += cls ? `<span class="${cls}">${escapeHtml(text)}</span>` : escapeHtml(text);
    last = index + text.length;
  }
  html += escapeHtml(source.slice(last));
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

function eventSnippet(name: string): { insert: string; caret: number } {
  const insert = `on('${name}') {\n  \n}\n`;
  return { insert, caret: insert.indexOf('\n') + 3 };
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

export function getCompletions(
  source: string,
  caret: number,
  manifest: CapabilityManifest,
  scriptNames: readonly string[],
  force = false,
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
      const snippet = eventSnippet(event.name);
      items.push({
        label: `on('${event.name}')`,
        detail: `${event.params} params`,
        kind: 'event',
        insert: snippet.insert,
        caret: snippet.caret,
      });
    }
  }

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
