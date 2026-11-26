import type { SourcePos, Token, TokenKind } from './types.ts';

export class ScriptError extends Error {
  readonly code: string;
  readonly pos: SourcePos;

  constructor(code: string, message: string, pos: SourcePos) {
    super(message);
    this.name = 'ScriptError';
    this.code = code;
    this.pos = pos;
  }
}

const KEYWORDS = new Set([
  'if',
  'else',
  'while',
  'on',
  'return',
  'break',
  'continue',
  'true',
  'false',
]);

const TWO_CHAR_PUNCT = new Set([
  '==',
  '!=',
  '<=',
  '>=',
  '&&',
  '||',
  '<<',
  '>>',
]);

const ONE_CHAR_PUNCT = new Set('{}()[];,=+-*/%!~&|^<>?:'.split(''));

function isDigit(ch: string): boolean {
  return ch >= '0' && ch <= '9';
}

function isIdentStart(ch: string): boolean {
  return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch === '_';
}

function isIdentPart(ch: string): boolean {
  return isIdentStart(ch) || isDigit(ch);
}

function isHexDigit(ch: string): boolean {
  return isDigit(ch) || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F');
}

export function lex(source: string): Token[] {
  const tokens: Token[] = [];
  let offset = 0;
  let line = 1;
  let column = 1;

  const pos = (): SourcePos => ({ line, column, offset });

  const advance = (count = 1): void => {
    for (let i = 0; i < count; i++) {
      if (source[offset] === '\n') {
        line++;
        column = 1;
      } else {
        column++;
      }
      offset++;
    }
  };

  const peek = (ahead = 0): string => source[offset + ahead] ?? '';

  const push = (kind: TokenKind, value: string, start: SourcePos): void => {
    tokens.push({ kind, value, pos: start });
  };

  while (offset < source.length) {
    const ch = peek();

    if (ch === ' ' || ch === '\t' || ch === '\r' || ch === '\n') {
      advance();
      continue;
    }

    if (ch === '/' && peek(1) === '/') {
      while (offset < source.length && peek() !== '\n') advance();
      continue;
    }

    if (ch === '/' && peek(1) === '*') {
      const start = pos();
      advance(2);
      while (offset < source.length && !(peek() === '*' && peek(1) === '/')) advance();
      if (offset >= source.length) {
        throw new ScriptError('E_COMMENT', 'unterminated block comment', start);
      }
      advance(2);
      continue;
    }

    const start = pos();

    if (isDigit(ch) || (ch === '.' && isDigit(peek(1)))) {
      let raw = '';
      if (ch === '0' && (peek(1) === 'x' || peek(1) === 'X')) {
        raw += peek();
        advance();
        raw += peek();
        advance();
        if (!isHexDigit(peek())) {
          throw new ScriptError('E_NUMBER', 'hex literal has no digits', start);
        }
        while (isHexDigit(peek())) {
          raw += peek();
          advance();
        }
      } else {
        while (isDigit(peek())) {
          raw += peek();
          advance();
        }
        if (peek() === '.') {
          raw += peek();
          advance();
          while (isDigit(peek())) {
            raw += peek();
            advance();
          }
        }
      }
      push('number', raw, start);
      continue;
    }

    if (ch === "'" || ch === '"') {
      const quote = ch;
      advance();
      let value = '';
      let closed = false;
      while (offset < source.length) {
        const c = peek();
        if (c === quote) {
          advance();
          closed = true;
          break;
        }
        if (c === '\n') {
          throw new ScriptError('E_STRING', 'unterminated string', start);
        }
        if (c === '\\') {
          advance();
          const esc = peek();
          if (!esc) break;
          if (esc === 'n') value += '\n';
          else if (esc === 'r') value += '\r';
          else if (esc === 't') value += '\t';
          else if (esc === '\\') value += '\\';
          else if (esc === "'") value += "'";
          else if (esc === '"') value += '"';
          else if (esc === 'x') {
            advance();
            const hi = peek();
            const lo = peek(1);
            if (!isHexDigit(hi) || !isHexDigit(lo)) {
              throw new ScriptError('E_STRING', 'invalid \\x escape', start);
            }
            value += String.fromCharCode(Number.parseInt(hi + lo, 16));
            advance();
            advance();
            continue;
          } else {
            value += esc;
          }
          advance();
          continue;
        }
        value += c;
        advance();
      }
      if (!closed) throw new ScriptError('E_STRING', 'unterminated string', start);
      push('string', value, start);
      continue;
    }

    if (ch === '$') {
      advance();
      let name = '';
      const first = peek();
      if (!isIdentStart(first)) {
        throw new ScriptError('E_VAR', 'expected variable name after $', start);
      }
      name += first;
      advance();
      while (isIdentPart(peek())) {
        name += peek();
        advance();
      }
      push('identifier', `$${name}`, start);
      continue;
    }

    if (isIdentStart(ch)) {
      let name = '';
      while (isIdentPart(peek())) {
        name += peek();
        advance();
      }
      push(KEYWORDS.has(name) ? 'keyword' : 'identifier', name, start);
      continue;
    }

    const two = ch + peek(1);
    if (TWO_CHAR_PUNCT.has(two)) {
      advance(2);
      push('punct', two, start);
      continue;
    }

    if (ONE_CHAR_PUNCT.has(ch)) {
      advance();
      push('punct', ch, start);
      continue;
    }

    throw new ScriptError('E_CHAR', `unexpected character '${ch}'`, start);
  }

  tokens.push({ kind: 'eof', value: '', pos: pos() });
  return tokens;
}
