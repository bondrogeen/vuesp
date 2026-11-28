import { lex, ScriptError } from './lexer.ts';
import type {
  ArrayExpr,
  AssignStmt,
  BinaryOp,
  CallExpr,
  Expr,
  IfStmt,
  LValue,
  NumberExpr,
  OnParam,
  OnStmt,
  Program,
  Stmt,
  StringExpr,
  Token,
  TokenKind,
  VarExpr,
  VarSlot,
  VarType,
  WhileStmt,
} from './types.ts';
import { encodeVar, VAR_TYPE_CODE } from './types.ts';

const BINARY_PRECEDENCE: Readonly<Record<BinaryOp, number>> = {
  '||': 1,
  '&&': 2,
  '|': 3,
  '^': 4,
  '&': 5,
  '==': 6,
  '!=': 6,
  '<': 7,
  '<=': 7,
  '>': 7,
  '>=': 7,
  '<<': 8,
  '>>': 8,
  '+': 9,
  '-': 9,
  '*': 10,
  '/': 10,
  '%': 10,
};

const WAIT_UNITS = new Set(['ms', 's', 'm', 'h']);
const SLOT_RE = /^\$([uifsapce])(\d+)$/;

function makeVarSlot(type: VarType, index: number): VarSlot {
  return { type, index, encoded: encodeVar(type, index) };
}

function parseSlot(name: string): VarSlot | null {
  const match = SLOT_RE.exec(name);
  if (!match) return null;
  const type = match[1] as VarType;
  if (!(type in VAR_TYPE_CODE)) return null;
  return makeVarSlot(type, Number.parseInt(match[2], 10));
}

export class Parser {
  private readonly tokens: Token[];
  private index = 0;
  private handlerScope: Map<string, number> | null = null;

  constructor(source: string) {
    this.tokens = lex(source);
  }

  parseProgram(): Program {
    const body: Stmt[] = [];
    while (!this.atKind('eof')) {
      this.skipSemicolons();
      if (this.atKind('eof')) break;
      body.push(this.parseStatement());
    }
    return { body };
  }

  private peek(ahead = 0): Token {
    return this.tokens[Math.min(this.index + ahead, this.tokens.length - 1)];
  }

  private next(): Token {
    const token = this.peek();
    if (token.kind !== 'eof') this.index++;
    return token;
  }

  private at(value: string): boolean {
    return this.peek().value === value;
  }

  private atKind(kind: TokenKind): boolean {
    return this.peek().kind === kind;
  }

  private eat(value: string): boolean {
    if (!this.at(value)) return false;
    this.next();
    return true;
  }

  private expect(value: string): Token {
    if (!this.at(value)) {
      throw new ScriptError(
        'E_SYNTAX',
        `expected '${value}' but found '${this.peek().value || 'end of file'}'`,
        this.peek().pos,
      );
    }
    return this.next();
  }

  private expectKind(kind: TokenKind): Token {
    if (!this.atKind(kind)) {
      throw new ScriptError(
        'E_SYNTAX',
        `expected ${kind} but found '${this.peek().value || 'end of file'}'`,
        this.peek().pos,
      );
    }
    return this.next();
  }

  private atKeyword(value: string): boolean {
    const token = this.peek();
    return token.kind === 'keyword' && token.value === value;
  }

  private eatKeyword(value: string): boolean {
    if (!this.atKeyword(value)) return false;
    this.next();
    return true;
  }

  private expectKeyword(value: string): Token {
    if (!this.atKeyword(value)) {
      throw new ScriptError(
        'E_SYNTAX',
        `expected '${value}' but found '${this.peek().value || 'end of file'}'`,
        this.peek().pos,
      );
    }
    return this.next();
  }

  private skipSemicolons(): void {
    while (this.eat(';')) {
      /* optional separators after blocks */
    }
  }

  private parseStatement(): Stmt {
    if (this.atKeyword('if')) return this.parseIf();
    if (this.atKeyword('while')) return this.parseWhile();
    if (this.atKeyword('on')) return this.parseOn();
    if (this.atKeyword('return')) return this.parseReturn();
    if (this.atKeyword('break')) return this.parseBreak();
    if (this.atKeyword('continue')) return this.parseContinue();
    return this.parseAssignmentOrExpr();
  }

  private parseBlock(): Stmt[] {
    this.expect('{');
    const body: Stmt[] = [];
    while (!this.at('}')) {
      if (this.atKind('eof')) {
        throw new ScriptError('E_BLOCK', 'unterminated block', this.peek().pos);
      }
      this.skipSemicolons();
      if (this.at('}')) break;
      body.push(this.parseStatement());
    }
    this.expect('}');
    return body;
  }

  private parseIf(): IfStmt {
    const start = this.expectKeyword('if').pos;
    this.expect('(');
    const cond = this.parseExpr();
    this.expect(')');
    const then = this.parseBlock();
    let elseBody: Stmt[] | undefined;
    if (this.eatKeyword('else')) {
      if (this.atKeyword('if')) {
        elseBody = [this.parseIf()];
      } else {
        elseBody = this.parseBlock();
      }
    }
    this.skipSemicolons();
    return { kind: 'if', cond, then, else: elseBody, pos: start };
  }

  private parseWhile(): WhileStmt {
    const start = this.expectKeyword('while').pos;
    this.expect('(');
    const cond = this.parseExpr();
    this.expect(')');
    const body = this.parseBlock();
    this.skipSemicolons();
    return { kind: 'while', cond, body, pos: start };
  }

  private parseOn(): OnStmt {
    const start = this.expectKeyword('on').pos;
    this.expect('(');
    const event = this.expectKind('string');

    if (this.eat(',')) {
      this.expect('(');
      const params: OnParam[] = [];
      if (!this.at(')')) {
        do {
          const name = this.expectKind('identifier');
          params.push({ name: name.value, pos: name.pos });
        } while (this.eat(','));
      }
      this.expect(')');
      this.expect('=>');

      const scope = new Map<string, number>();
      for (let i = 0; i < params.length; i++) scope.set(params[i].name, i);
      const previous = this.handlerScope;
      this.handlerScope = scope;
      const body = this.parseBlock();
      this.handlerScope = previous;
      this.expect(')');
      this.skipSemicolons();
      return { kind: 'on', event: event.value, params, body, pos: start };
    }

    this.expect(')');
    const body = this.parseBlock();
    this.skipSemicolons();
    return { kind: 'on', event: event.value, params: [], body, pos: start };
  }

  private parseReturn(): Stmt {
    const start = this.expectKeyword('return').pos;
    this.eat(';');
    return { kind: 'return', pos: start };
  }

  private parseBreak(): Stmt {
    const start = this.expectKeyword('break').pos;
    this.eat(';');
    return { kind: 'break', pos: start };
  }

  private parseContinue(): Stmt {
    const start = this.expectKeyword('continue').pos;
    this.eat(';');
    return { kind: 'continue', pos: start };
  }

  private parseAssignmentOrExpr(): Stmt {
    const start = this.peek().pos;
    const expr = this.parseExpr();
    if (this.eat('=')) {
      const target = this.toLValue(expr);
      const value = this.parseExpr();
      this.expect(';');
      const stmt: AssignStmt = { kind: 'assign', target, value, pos: start };
      return stmt;
    }
    this.expect(';');
    return { kind: 'expr', expr, pos: start };
  }

  private toLValue(expr: Expr): LValue {
    if (expr.kind === 'var') {
      if (expr.slot) {
        return { kind: 'var', name: expr.name, slot: expr.slot, pos: expr.pos };
      }
      return { kind: 'data', name: expr.name, pos: expr.pos };
    }
    if (expr.kind === 'dataVar') {
      return { kind: 'data', name: expr.name, pos: expr.pos };
    }
    if (expr.kind === 'index') {
      return { kind: 'index', array: expr.array, index: expr.index, pos: expr.pos };
    }
    throw new ScriptError('E_LVALUE', 'left side is not assignable', expr.pos);
  }

  private parseExpr(): Expr {
    return this.parseTernary();
  }

  private parseTernary(): Expr {
    const cond = this.parseBinary(0);
    if (!this.eat('?')) return cond;
    const thenExpr = this.parseExpr();
    this.expect(':');
    const elseExpr = this.parseExpr();
    return { kind: 'ternary', cond, then: thenExpr, else: elseExpr, pos: cond.pos };
  }

  private parseBinary(minPrecedence: number): Expr {
    let left = this.parseUnary();
    while (true) {
      const token = this.peek();
      if (token.kind !== 'punct') break;
      const precedence = BINARY_PRECEDENCE[token.value as BinaryOp];
      if (precedence === undefined || precedence < minPrecedence) break;
      const op = token.value as BinaryOp;
      this.next();
      const right = this.parseBinary(precedence + 1);
      left = { kind: 'binary', op, left, right, pos: left.pos };
    }
    return left;
  }

  private parseUnary(): Expr {
    const token = this.peek();
    if (token.kind === 'punct' && (token.value === '!' || token.value === '-' || token.value === '~')) {
      this.next();
      if (token.value === '-' && this.peek().kind === 'number') {
        const numberToken = this.next();
        return {
          kind: 'number',
          value: -Number(numberToken.value),
          isFloat: numberToken.value.includes('.'),
          pos: token.pos,
        };
      }
      return {
        kind: 'unary',
        op: token.value as '!' | '-' | '~',
        expr: this.parseUnary(),
        pos: token.pos,
      };
    }
    return this.parsePostfix();
  }

  private parsePostfix(): Expr {
    let expr = this.parsePrimary();
    while (this.eat('[')) {
      const index = this.parseExpr();
      this.expect(']');
      expr = { kind: 'index', array: expr, index, pos: expr.pos };
    }
    return expr;
  }

  private parsePrimary(): Expr {
    const token = this.peek();

    if (token.kind === 'number') {
      this.next();
      const isFloat = token.value.includes('.');
      return { kind: 'number', value: Number(token.value), isFloat, pos: token.pos };
    }

    if (token.kind === 'string') {
      this.next();
      const stringExpr: StringExpr = { kind: 'string', value: token.value, pos: token.pos };
      return stringExpr;
    }

    if (token.kind === 'keyword' && (token.value === 'true' || token.value === 'false')) {
      this.next();
      return { kind: 'bool', value: token.value === 'true', pos: token.pos };
    }

    if (token.kind === 'identifier') {
      this.next();
      const paramIndex = this.handlerScope?.get(token.value);
      if (paramIndex !== undefined) {
        const varExpr: VarExpr = {
          kind: 'var',
          name: token.value,
          slot: makeVarSlot('e', paramIndex),
          pos: token.pos,
        };
        return varExpr;
      }
      if (token.value.startsWith('$')) {
        const slot = parseSlot(token.value);
        if (slot) {
          const varExpr: VarExpr = {
            kind: 'var',
            name: token.value,
            slot,
            pos: token.pos,
          };
          return varExpr;
        }
        return { kind: 'dataVar', name: token.value, pos: token.pos };
      }
      if (!this.at('(')) {
        throw new ScriptError(
          'E_SYNTAX',
          `expected '(' after function '${token.value}'`,
          this.peek().pos,
        );
      }
      return this.parseCall(token);
    }

    if (this.eat('(')) {
      const expr = this.parseExpr();
      this.expect(')');
      return expr;
    }

    if (this.at('[')) {
      return this.parseArrayLiteral();
    }

    throw new ScriptError(
      'E_SYNTAX',
      `unexpected token '${token.value || 'end of file'}'`,
      token.pos,
    );
  }

  private parseCall(nameToken: Token): CallExpr {
    this.expect('(');
    const args: Expr[] = [];

    if (nameToken.value === 'wait' && this.peek().kind === 'number') {
      const numberToken = this.next();
      const numberExpr: NumberExpr = {
        kind: 'number',
        value: Number(numberToken.value),
        isFloat: numberToken.value.includes('.'),
        pos: numberToken.pos,
      };
      let unit = 's';
      if (this.peek().kind === 'identifier' && WAIT_UNITS.has(this.peek().value)) {
        unit = this.next().value;
      }
      this.expect(')');
      return {
        kind: 'call',
        name: 'wait',
        args: [numberExpr, { kind: 'string', value: unit, pos: numberToken.pos }],
        pos: nameToken.pos,
      };
    }

    if (!this.at(')')) {
      do {
        args.push(this.parseExpr());
      } while (this.eat(','));
    }
    this.expect(')');
    return { kind: 'call', name: nameToken.value, args, pos: nameToken.pos };
  }

  private parseArrayLiteral(): ArrayExpr {
    const start = this.expect('[').pos;
    const items: Expr[] = [];
    if (!this.at(']')) {
      do {
        items.push(this.parseExpr());
      } while (this.eat(','));
    }
    this.expect(']');
    return { kind: 'array', items, pos: start };
  }
}

export function parse(source: string): Program {
  return new Parser(source).parseProgram();
}
