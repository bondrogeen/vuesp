import type {
  BinaryOp,
  CallExpr,
  Expr,
  LValue,
  Program,
  Stmt,
} from './types.ts';

const BINARY_PRECEDENCE: Readonly<Record<BinaryOp, number>> = {
  '||': 2,
  '&&': 3,
  '|': 4,
  '^': 5,
  '&': 6,
  '==': 7,
  '!=': 7,
  '<': 8,
  '<=': 8,
  '>': 8,
  '>=': 8,
  '<<': 9,
  '>>': 9,
  '+': 10,
  '-': 10,
  '*': 11,
  '/': 11,
  '%': 11,
};

const TERNARY_PRECEDENCE = 1;
const UNARY_PRECEDENCE = 12;
const POSTFIX_PRECEDENCE = 13;
const PRIMARY_PRECEDENCE = 14;

function escapeString(value: string): string {
  let out = "'";
  for (const ch of value) {
    if (ch === '\\') out += '\\\\';
    else if (ch === "'") out += "\\'";
    else if (ch === '\n') out += '\\n';
    else if (ch === '\r') out += '\\r';
    else if (ch === '\t') out += '\\t';
    else {
      const code = ch.codePointAt(0) ?? 0;
      if (code < 0x20 || code === 0x7f) {
        out += `\\x${code.toString(16).padStart(2, '0')}`;
      } else {
        out += ch;
      }
    }
  }
  return out + "'";
}

function formatFloat(value: number): string {
  if (!Number.isFinite(value)) return '0.0';
  const text = String(value);
  return text.includes('.') || text.includes('e') ? text : `${text}.0`;
}

function waitMilliseconds(call: CallExpr): number | null {
  if (call.name !== 'wait' || call.args.length !== 2) return null;
  const [value, unit] = call.args;
  if (value.kind !== 'number' || unit.kind !== 'string') return null;
  const multiplier =
    unit.value === 'ms' ? 1 : unit.value === 's' ? 1000 : unit.value === 'm' ? 60000 : unit.value === 'h' ? 3600000 : 0;
  if (multiplier === 0) return null;
  const ms = value.value * multiplier;
  return Number.isInteger(ms) ? ms : null;
}

function precedence(expr: Expr): number {
  switch (expr.kind) {
    case 'ternary':
      return TERNARY_PRECEDENCE;
    case 'binary':
      return BINARY_PRECEDENCE[expr.op];
    case 'unary':
      return UNARY_PRECEDENCE;
    case 'call':
    case 'index':
      return POSTFIX_PRECEDENCE;
    default:
      return PRIMARY_PRECEDENCE;
  }
}

export function printExpr(expr: Expr, minPrecedence = 0): string {
  const prec = precedence(expr);
  let text: string;

  switch (expr.kind) {
    case 'number':
      text = expr.isFloat ? formatFloat(expr.value) : String(expr.value);
      break;
    case 'string':
      text = escapeString(expr.value);
      break;
    case 'bool':
      text = expr.value ? 'true' : 'false';
      break;
    case 'var':
    case 'dataVar':
      text = expr.name;
      break;
    case 'unary':
      text = `${expr.op}${printExpr(expr.expr, UNARY_PRECEDENCE)}`;
      break;
    case 'binary': {
      const p = BINARY_PRECEDENCE[expr.op];
      text = `${printExpr(expr.left, p)} ${expr.op} ${printExpr(expr.right, p + 1)}`;
      break;
    }
    case 'ternary':
      text = `${printExpr(expr.cond, TERNARY_PRECEDENCE + 1)} ? ${printExpr(
        expr.then,
        TERNARY_PRECEDENCE + 1,
      )} : ${printExpr(expr.else, TERNARY_PRECEDENCE)}`;
      break;
    case 'call': {
      const waitMs = waitMilliseconds(expr);
      if (waitMs !== null) {
        text = `wait(${waitMs}ms)`;
      } else {
        text = `${expr.name}(${expr.args.map((arg) => printExpr(arg)).join(', ')})`;
      }
      break;
    }
    case 'index':
      text = `${printExpr(expr.array, POSTFIX_PRECEDENCE)}[${printExpr(expr.index)}]`;
      break;
    case 'array':
      text = `[${expr.items.map((item) => printExpr(item)).join(', ')}]`;
      break;
  }

  return prec < minPrecedence ? `(${text})` : text;
}

function printTarget(target: LValue): string {
  switch (target.kind) {
    case 'var':
      return target.name;
    case 'data':
      return target.name;
    case 'index':
      return `${printExpr(target.array, POSTFIX_PRECEDENCE)}[${printExpr(target.index)}]`;
  }
}

function indent(level: number): string {
  return '  '.repeat(level);
}

function printBlock(body: readonly Stmt[], level: number): string {
  if (body.length === 0) return `${indent(level)}{\n${indent(level)}}`;
  return `${indent(level)}{\n${printStatements(body, level + 1)}${indent(level)}}`;
}

function printStatements(body: readonly Stmt[], level: number): string {
  return body.map((stmt) => printStmt(stmt, level)).join('');
}

function printIf(stmt: Extract<Stmt, { kind: 'if' }>, level: number): string {
  let text = `${indent(level)}if (${printExpr(stmt.cond)}) ${printBlock(stmt.then, level)}`;
  if (stmt.else && stmt.else.length > 0) {
    if (stmt.else.length === 1 && stmt.else[0].kind === 'if') {
      text += ` else ${printIf(stmt.else[0], level).trimStart()}`;
    } else {
      text += ` else ${printBlock(stmt.else, level)}`;
    }
  }
  return text;
}

function printStmt(stmt: Stmt, level: number): string {
  switch (stmt.kind) {
    case 'assign':
      return `${indent(level)}${printTarget(stmt.target)} = ${printExpr(stmt.value)};\n`;
    case 'expr':
      return `${indent(level)}${printExpr(stmt.expr)};\n`;
    case 'if':
      return `${printIf(stmt, level)}\n`;
    case 'while':
      return `${indent(level)}while (${printExpr(stmt.cond)}) ${printBlock(stmt.body, level)}\n`;
    case 'on': {
      if (stmt.params.length > 0) {
        const args = stmt.params.map((param) => param.name).join(', ');
        return `${indent(level)}on(${escapeString(stmt.event)}, (${args}) => ${printBlock(stmt.body, level)});\n`;
      }
      return `${indent(level)}on(${escapeString(stmt.event)}) ${printBlock(stmt.body, level)}\n`;
    }
    case 'return':
      return `${indent(level)}return;\n`;
    case 'break':
      return `${indent(level)}break;\n`;
    case 'continue':
      return `${indent(level)}continue;\n`;
  }
}

export function printProgram(program: Program): string {
  if (program.body.length === 0) return '';
  const parts: string[] = [];
  for (const stmt of program.body) {
    parts.push(printStmt(stmt, 0).trimEnd());
  }
  return `${parts.join('\n')}\n`;
}
