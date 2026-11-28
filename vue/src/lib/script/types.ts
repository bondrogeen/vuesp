// Shared types for the ScriptRunner bytecode compiler and decompiler.
// This module is intentionally free of runtime enums so Node's TypeScript
// type stripping can execute it directly in tests.

import {
  SCRIPT_EVENTS,
  SCRIPT_EVENT_ARG_NAMES,
  SCRIPT_FUNCTIONS,
  SCRIPT_LIMITS,
} from '../../generated/scriptManifest.ts';

export interface SourcePos {
  readonly line: number;
  readonly column: number;
  readonly offset: number;
}

export interface Token {
  readonly kind: TokenKind;
  readonly value: string;
  readonly pos: SourcePos;
}

export type TokenKind =
  | 'eof'
  | 'identifier'
  | 'number'
  | 'string'
  | 'punct'
  | 'keyword';

export const VAR_TYPES = ['u', 'i', 'f', 's', 'a', 'p', 'e', 'c'] as const;
export type VarType = (typeof VAR_TYPES)[number];

export const VAR_TYPE_CODE: Readonly<Record<VarType, number>> = {
  u: 0,
  i: 1,
  f: 2,
  s: 3,
  a: 4,
  p: 5,
  e: 6,
  c: 7,
};

export function encodeVar(type: VarType, index: number): number {
  return (VAR_TYPE_CODE[type] << 4) | index;
}

export function decodeVar(code: number): { type: VarType; index: number } {
  const type = VAR_TYPES[(code >> 4) & 0x0f];
  if (!type) throw new Error(`Invalid variable encoding 0x${code.toString(16)}`);
  return { type, index: code & 0x0f };
}

export interface VarSlot {
  readonly type: VarType;
  readonly index: number;
  readonly encoded: number;
}

export type Expr =
  | NumberExpr
  | StringExpr
  | BoolExpr
  | VarExpr
  | DataVarExpr
  | UnaryExpr
  | BinaryExpr
  | TernaryExpr
  | CallExpr
  | IndexExpr
  | ArrayExpr;

export interface NumberExpr {
  readonly kind: 'number';
  readonly value: number;
  readonly isFloat: boolean;
  readonly pos: SourcePos;
}

export interface StringExpr {
  readonly kind: 'string';
  readonly value: string;
  readonly pos: SourcePos;
}

export interface BoolExpr {
  readonly kind: 'bool';
  readonly value: boolean;
  readonly pos: SourcePos;
}

export interface VarExpr {
  readonly kind: 'var';
  readonly name: string;
  readonly slot?: VarSlot;
  readonly pos: SourcePos;
}

export interface DataVarExpr {
  readonly kind: 'dataVar';
  readonly name: string;
  readonly dataKind?: number;
  readonly pos: SourcePos;
}

export interface UnaryExpr {
  readonly kind: 'unary';
  readonly op: '-' | '!' | '~';
  readonly expr: Expr;
  readonly pos: SourcePos;
}

export interface BinaryExpr {
  readonly kind: 'binary';
  readonly op: BinaryOp;
  readonly left: Expr;
  readonly right: Expr;
  readonly pos: SourcePos;
}

export type BinaryOp =
  | '+'
  | '-'
  | '*'
  | '/'
  | '%'
  | '<<'
  | '>>'
  | '<'
  | '<='
  | '>'
  | '>='
  | '=='
  | '!='
  | '&'
  | '^'
  | '|'
  | '&&'
  | '||';

export interface TernaryExpr {
  readonly kind: 'ternary';
  readonly cond: Expr;
  readonly then: Expr;
  readonly else: Expr;
  readonly pos: SourcePos;
}

export interface CallExpr {
  readonly kind: 'call';
  readonly name: string;
  readonly args: readonly Expr[];
  readonly pos: SourcePos;
}

export interface IndexExpr {
  readonly kind: 'index';
  readonly array: Expr;
  readonly index: Expr;
  readonly pos: SourcePos;
}

export interface ArrayExpr {
  readonly kind: 'array';
  readonly items: readonly Expr[];
  readonly pos: SourcePos;
}

export type LValue =
  | VarLValue
  | DataLValue
  | IndexLValue;

export interface VarLValue {
  readonly kind: 'var';
  readonly name: string;
  readonly slot?: VarSlot;
  readonly pos: SourcePos;
}

export interface DataLValue {
  readonly kind: 'data';
  readonly name: string;
  readonly pos: SourcePos;
}

export interface IndexLValue {
  readonly kind: 'index';
  readonly array: Expr;
  readonly index: Expr;
  readonly pos: SourcePos;
}

export type Stmt =
  | AssignStmt
  | ExprStmt
  | IfStmt
  | WhileStmt
  | OnStmt
  | ReturnStmt
  | BreakStmt
  | ContinueStmt;

export interface AssignStmt {
  readonly kind: 'assign';
  readonly target: LValue;
  readonly value: Expr;
  readonly pos: SourcePos;
}

export interface ExprStmt {
  readonly kind: 'expr';
  readonly expr: Expr;
  readonly pos: SourcePos;
}

export interface IfStmt {
  readonly kind: 'if';
  readonly cond: Expr;
  readonly then: readonly Stmt[];
  readonly else?: readonly Stmt[];
  readonly pos: SourcePos;
}

export interface WhileStmt {
  readonly kind: 'while';
  readonly cond: Expr;
  readonly body: readonly Stmt[];
  readonly pos: SourcePos;
}

export const EVENT_ARG_NAMES: Readonly<Record<string, readonly string[]>> =
  SCRIPT_EVENT_ARG_NAMES;

export const getEventArgNames = (name: string): readonly string[] =>
  EVENT_ARG_NAMES[name] ?? [];

export interface OnParam {
  readonly name: string;
  readonly pos: SourcePos;
}

export interface OnStmt {
  readonly kind: 'on';
  readonly event: string;
  readonly params: readonly OnParam[];
  readonly body: readonly Stmt[];
  readonly pos: SourcePos;
}

export interface ReturnStmt {
  readonly kind: 'return';
  readonly pos: SourcePos;
}

export interface BreakStmt {
  readonly kind: 'break';
  readonly pos: SourcePos;
}

export interface ContinueStmt {
  readonly kind: 'continue';
  readonly pos: SourcePos;
}

export interface Program {
  readonly body: readonly Stmt[];
}

export interface CompileError {
  readonly code: string;
  readonly message: string;
  readonly pos: SourcePos;
}

export interface CompileWarning {
  readonly code: string;
  readonly message: string;
  readonly pos: SourcePos;
}

export interface CompileStats {
  readonly codeBytes: number;
  readonly constCount: number;
  readonly blockCount: number;
  readonly handlerCount: number;
  readonly stackMax: number;
}

export interface CompileResult {
  readonly ok: boolean;
  readonly errors: readonly CompileError[];
  readonly warnings: readonly CompileWarning[];
  readonly bytecode?: Uint8Array;
  readonly disassembly?: string;
  readonly source?: string;
  readonly stats?: CompileStats;
}

export interface FunctionSpec {
  readonly id: number;
  readonly name: string;
  readonly min: number;
  readonly max: number;
  readonly ret: 'none' | 'int' | 'uint' | 'float' | 'string' | 'array';
}

export interface EventSpec {
  readonly id: number;
  readonly name: string;
  readonly params: number;
}

export interface DataVarSpec {
  readonly name: string;
  readonly kind: 'int' | 'uint' | 'float' | 'string';
}

export interface VarCounts {
  readonly u: number;
  readonly i: number;
  readonly f: number;
  readonly s: number;
  readonly a: number;
  readonly p: number;
  readonly e: number;
  readonly c: number;
}

export interface CapabilityManifest {
  readonly vm: number;
  readonly maxScripts: number;
  readonly cacheSlots?: number;
  readonly maxCodeBytes: number;
  readonly maxStack: number;
  readonly vars: VarCounts;
  readonly maxStringLen: number;
  readonly maxArrayLen: number;
  readonly functions: readonly FunctionSpec[];
  readonly events: readonly EventSpec[];
  readonly dataVars?: readonly DataVarSpec[];
  readonly limits: {
    readonly instrPerTick: number;
    readonly usPerTick: number;
    readonly updateBudgetUs: number;
  };
}

export const DEFAULT_MANIFEST: CapabilityManifest = {
  vm: 1,
  maxScripts: SCRIPT_LIMITS.maxSavedScripts,
  cacheSlots: SCRIPT_LIMITS.cacheSlots,
  maxCodeBytes: SCRIPT_LIMITS.maxCodeBytes,
  maxStack: SCRIPT_LIMITS.stackSize,
  vars: SCRIPT_LIMITS.vars,
  maxStringLen: SCRIPT_LIMITS.maxStringLen,
  maxArrayLen: SCRIPT_LIMITS.maxArrayLen,
  functions: SCRIPT_FUNCTIONS.map(({ id, name, min, max, ret }) => ({ id, name, min, max, ret })),
  events: SCRIPT_EVENTS.map(({ id, name, params }) => ({ id, name, params })),
  dataVars: [{ name: '$display', kind: 'string' }],
  limits: {
    instrPerTick: SCRIPT_LIMITS.instrPerTick,
    usPerTick: SCRIPT_LIMITS.usPerTick,
    updateBudgetUs: SCRIPT_LIMITS.updateBudgetUs,
  },
};

export function error(code: string, message: string, pos: SourcePos): CompileError {
  return { code, message, pos };
}

export function formatError(err: CompileError): string {
  return `${err.pos.line}:${err.pos.column} ${err.code}: ${err.message}`;
}
