import { ScriptError } from './lexer.ts';
import { Opcode, OPCODES } from './opcodes.ts';
import type { OpcodeName } from './opcodes.ts';
import { parse } from './parser.ts';
import { printProgram } from './printer.ts';
import { disassemble } from './disassembler.ts';
import {
  BLOCK_ELSE,
  BLOCK_HANDLER,
  BLOCK_IF,
  BLOCK_TERNARY,
  BLOCK_WHILE,
  CONST_ARRAY,
  CONST_NAME,
  CONST_STRING,
  ContainerError,
  HEADER_SIZE,
  writeContainer,
  writeF32,
  writeU16,
  writeU32,
} from './container.ts';
import type {
  BlockEntry,
  BytecodeContainer,
  BytecodeHeader,
  ConstantEntry,
  HandlerEntry,
} from './container.ts';
import type {
  ArrayExpr,
  BinaryOp,
  CallExpr,
  CapabilityManifest,
  CompileResult,
  CompileStats,
  CompileWarning,
  DataVarSpec,
  Expr,
  FunctionSpec,
  LValue,
  NumberExpr,
  OnStmt,
  Program,
  SourcePos,
  Stmt,
  VarSlot,
} from './types.ts';
import { DEFAULT_MANIFEST } from './types.ts';

export const KIND_INT = 0;
export const KIND_UINT = 1;
export const KIND_FLOAT = 2;
export const KIND_STRING = 3;

export interface CompileOptions {
  readonly manifest?: CapabilityManifest;
  readonly includeSourceMap?: boolean;
}

interface Fixup {
  readonly at: number;
  readonly label: string;
}

interface LoopScope {
  readonly continueLabel: string;
  readonly breakLabel: string;
}

interface ValidationContext {
  readonly inLoop: boolean;
  readonly inHandler: boolean;
  readonly topLevel: boolean;
  readonly events: Set<string>;
}

interface BuildResult {
  readonly bytecode: Uint8Array;
  readonly stats: CompileStats;
}

const BUILTIN_NAMES = new Set(['log', 'wait', 'call', 'set', 'get', 'len', 'chr', 'ord']);
const RESERVED_HANDLER_PARAMS = new Set([
  ...BUILTIN_NAMES,
  'if', 'else', 'while', 'on', 'return', 'break', 'continue', 'true', 'false',
]);
const MAX_LITERAL_STRING = 255;

function djb2(text: string): number {
  let hash = 5381;
  for (let i = 0; i < text.length; i++) {
    hash = ((hash << 5) + hash + text.charCodeAt(i)) >>> 0;
  }
  return hash >>> 0;
}

function operandSize(name: string): number {
  if (name === 'i8' || name === 'u8' || name === 'var8' || name === 'kind8' || name === 'argc8' || name === 'func8' || name === 'event8') {
    return 1;
  }
  if (name === 'i16' || name === 'u16' || name === 'const16' || name === 'name16') return 2;
  if (name === 'i32' || name === 'u32' || name === 'f32') return 4;
  throw new ScriptError('E_OPERAND', `unknown operand type ${name}`, { line: 1, column: 1, offset: 0 });
}

function dataKindNumber(kind: DataVarSpec['kind']): number {
  if (kind === 'string') return KIND_STRING;
  if (kind === 'float') return KIND_FLOAT;
  if (kind === 'uint') return KIND_UINT;
  return KIND_INT;
}

function isNumericLiteral(expr: Expr): expr is NumberExpr {
  return expr.kind === 'number' && !expr.isFloat;
}

function stringLiteralLength(expr: Expr): number {
  return expr.kind === 'string' ? expr.value.length : 0;
}

class ConstantPool {
  private readonly entries: ConstantEntry[] = [];
  private readonly index = new Map<string, number>();

  add(entry: ConstantEntry): number {
    const key = this.key(entry);
    const existing = this.index.get(key);
    if (existing !== undefined) return existing;
    const result = this.entries.length;
    this.entries.push(entry);
    this.index.set(key, result);
    return result;
  }

  addString(value: string): number {
    return this.add({ tag: CONST_STRING, value });
  }

  addName(value: string): number {
    return this.add({ tag: CONST_NAME, value });
  }

  addArray(bytes: Uint8Array): number {
    return this.add({ tag: CONST_ARRAY, value: bytes });
  }

  get all(): readonly ConstantEntry[] {
    return this.entries;
  }

  get count(): number {
    return this.entries.length;
  }

  private key(entry: ConstantEntry): string {
    if (entry.tag === CONST_STRING || entry.tag === CONST_NAME) {
      return `${entry.tag}:${entry.value as string}`;
    }
    if (entry.tag === CONST_ARRAY) {
      const bytes = entry.value as Uint8Array;
      return `${entry.tag}:${Array.from(bytes).join(',')}`;
    }
    return `${entry.tag}:${entry.value}`;
  }
}

export class BytecodeCompiler {
  private readonly manifest: CapabilityManifest;
  private readonly warnings: CompileWarning[];
  private readonly constants = new ConstantPool();
  private readonly code: number[] = [];
  private readonly blocks: BlockEntry[] = [];
  private readonly handlers: HandlerEntry[] = [];
  private readonly labels = new Map<string, number>();
  private readonly fixups: Fixup[] = [];
  private readonly loopStack: LoopScope[] = [];
  private labelCounter = 0;
  private stackDepth = 0;
  private stackMax = 0;

  constructor(manifest: CapabilityManifest, warnings: CompileWarning[]) {
    this.manifest = manifest;
    this.warnings = warnings;
  }

  compile(program: Program, canonicalSource: string): BuildResult {
    this.validateProgram(program);
    this.genProgram(program);
    this.patchFixups();

    if (this.code.length > this.manifest.maxCodeBytes) {
      throw new ScriptError(
        'E_CODE_SIZE',
        `code is ${this.code.length} bytes, limit is ${this.manifest.maxCodeBytes}`,
        { line: 1, column: 1, offset: 0 },
      );
    }
    if (this.stackMax > this.manifest.maxStack) {
      throw new ScriptError(
        'E_STACK',
        `stack needs ${this.stackMax}, limit is ${this.manifest.maxStack}`,
        { line: 1, column: 1, offset: 0 },
      );
    }
    if (this.constants.count > 0xffff) {
      throw new ScriptError('E_CONST_COUNT', 'too many constants', { line: 1, column: 1, offset: 0 });
    }

    const header: BytecodeHeader = {
      magic: 'VBC1',
      version: 1,
      flags: 0,
      headerSize: HEADER_SIZE,
      codeLength: this.code.length,
      constCount: this.constants.count,
      blockCount: this.blocks.length,
      handlerCount: this.handlers.length,
      entryOffset: 0,
      stackMax: this.stackMax,
      sourceHash: djb2(canonicalSource),
      crc32: 0,
    };
    const container: BytecodeContainer = {
      header,
      constants: this.constants.all,
      code: Uint8Array.from(this.code),
      blocks: this.blocks,
      handlers: this.handlers,
    };
    const bytecode = writeContainer(container);
    if (bytecode.length > this.manifest.maxCodeBytes) {
      throw new ScriptError(
        'E_CODE_SIZE',
        `container is ${bytecode.length} bytes, limit is ${this.manifest.maxCodeBytes}`,
        { line: 1, column: 1, offset: 0 },
      );
    }
    return {
      bytecode,
      stats: {
        codeBytes: this.code.length,
        constCount: this.constants.count,
        blockCount: this.blocks.length,
        handlerCount: this.handlers.length,
        stackMax: this.stackMax,
      },
    };
  }

  // ---------------------------------------------------------------------------
  // Validation

  private validateProgram(program: Program): void {
    const events = new Set<string>();
    for (const stmt of program.body) {
      this.validateStmt(stmt, {
        inLoop: false,
        inHandler: false,
        topLevel: true,
        events,
      });
    }
  }

  private validateStmt(stmt: Stmt, ctx: ValidationContext): void {
    switch (stmt.kind) {
      case 'on': {
        if (!ctx.topLevel) {
          throw new ScriptError('E_EVENT_SCOPE', 'on() is only allowed at top level', stmt.pos);
        }
        if (ctx.events.has(stmt.event)) {
          throw new ScriptError('E_DUPLICATE_EVENT', `duplicate handler for '${stmt.event}'`, stmt.pos);
        }
        const event = this.findEvent(stmt.event);
        if (!event) {
          throw new ScriptError('E_UNKNOWN_EVENT', `unknown event '${stmt.event}'`, stmt.pos);
        }
        const maxParams = Math.min(event.params, this.manifest.vars.e);
        if (stmt.params.length > maxParams) {
          throw new ScriptError(
            'E_EVENT_ARITY',
            `event '${stmt.event}' accepts at most ${maxParams} parameters`,
            stmt.pos,
          );
        }
        const paramNames = new Set<string>();
        for (const param of stmt.params) {
          if (paramNames.has(param.name)) {
            throw new ScriptError('E_DUPLICATE_PARAM', `duplicate parameter '${param.name}'`, param.pos);
          }
          paramNames.add(param.name);
          if (
            param.name.startsWith('$') ||
            RESERVED_HANDLER_PARAMS.has(param.name) ||
            this.manifest.functions.some((fn) => fn.name === param.name) ||
            this.manifest.events.some((item) => item.name === param.name)
          ) {
            throw new ScriptError('E_PARAM_NAME', `parameter name '${param.name}' is reserved`, param.pos);
          }
        }
        ctx.events.add(stmt.event);
        for (const child of stmt.body) {
          this.validateStmt(child, {
            inLoop: false,
            inHandler: true,
            topLevel: false,
            events: ctx.events,
          });
        }
        return;
      }
      case 'while': {
        this.validateExpr(stmt.cond, true);
        for (const child of stmt.body) {
          this.validateStmt(child, { ...ctx, inLoop: true, topLevel: false });
        }
        return;
      }
      case 'if': {
        this.validateExpr(stmt.cond, true);
        for (const child of stmt.then) {
          this.validateStmt(child, { ...ctx, topLevel: false });
        }
        for (const child of stmt.else ?? []) {
          this.validateStmt(child, { ...ctx, topLevel: false });
        }
        return;
      }
      case 'assign': {
        this.validateTarget(stmt.target);
        this.validateExpr(stmt.value, true);
        if (stmt.target.kind === 'var' && stmt.target.slot?.type === 'p' && isNumericLiteral(stmt.value)) {
          const value = stmt.value.value;
          if (value < 0 || value > 255) {
            throw new ScriptError('E_PORT_RANGE', 'port value must be 0..255', stmt.value.pos);
          }
        }
        if (stmt.target.kind === 'var' && stmt.target.slot?.type === 's') {
          const len = stringLiteralLength(stmt.value);
          if (len > this.manifest.maxStringLen) {
            this.warnings.push({
              code: 'W_STRING_TRUNCATED',
              message: `string literal is longer than ${this.manifest.maxStringLen} and will be truncated`,
              pos: stmt.value.pos,
            });
          }
        }
        return;
      }
      case 'expr':
        this.validateExpr(stmt.expr, false);
        return;
      case 'return':
        if (!ctx.inHandler) {
          throw new ScriptError('E_RETURN_SCOPE', 'return is only allowed in an event handler', stmt.pos);
        }
        return;
      case 'break':
        if (!ctx.inLoop) {
          throw new ScriptError('E_LOOP_SCOPE', 'break is only allowed in a loop', stmt.pos);
        }
        return;
      case 'continue':
        if (!ctx.inLoop) {
          throw new ScriptError('E_LOOP_SCOPE', 'continue is only allowed in a loop', stmt.pos);
        }
        return;
    }
  }

  private validateTarget(target: LValue): void {
    if (target.kind === 'var') {
      if (!target.slot) {
        throw new ScriptError('E_LVALUE', 'invalid variable target', target.pos);
      }
      if (target.slot.type === 'e' || target.slot.type === 'c') {
        throw new ScriptError('E_READONLY', `$${target.slot.type}${target.slot.index} is read-only`, target.pos);
      }
      this.validateSlot(target.slot, true, target.pos);
      return;
    }
    if (target.kind === 'data') {
      return;
    }
    if (!this.isArrayExpression(target.array)) {
      throw new ScriptError('E_ARRAY_EXPR', 'indexed assignment requires an array slot', target.pos);
    }
    this.validateExpr(target.array, true);
    this.validateExpr(target.index, true);
  }

  private validateExpr(expr: Expr, usedAsExpression: boolean): void {
    switch (expr.kind) {
      case 'number':
        this.validateNumber(expr);
        return;
      case 'string':
        if (expr.value.length > MAX_LITERAL_STRING) {
          throw new ScriptError('E_STRING_LEN', `string literal exceeds ${MAX_LITERAL_STRING} characters`, expr.pos);
        }
        return;
      case 'bool':
        return;
      case 'var':
        if (expr.slot) this.validateSlot(expr.slot, false, expr.pos);
        return;
      case 'dataVar':
        return;
      case 'unary':
        this.validateExpr(expr.expr, true);
        return;
      case 'binary':
        this.validateExpr(expr.left, true);
        this.validateExpr(expr.right, true);
        return;
      case 'ternary':
        this.validateExpr(expr.cond, true);
        this.validateExpr(expr.then, true);
        this.validateExpr(expr.else, true);
        return;
      case 'call':
        this.validateCall(expr, usedAsExpression);
        return;
      case 'index': {
        if (!this.isArrayExpression(expr.array)) {
          throw new ScriptError('E_ARRAY_EXPR', 'indexing requires an array expression', expr.pos);
        }
        this.validateExpr(expr.array, true);
        this.validateExpr(expr.index, true);
        return;
      }
      case 'array':
        this.validateArrayLiteral(expr);
        return;
    }
  }

  private validateCall(call: CallExpr, usedAsExpression: boolean): void {
    if (!BUILTIN_NAMES.has(call.name)) {
      const fn = this.findFunction(call.name);
      if (!fn) {
        throw new ScriptError('E_UNKNOWN_FUNCTION', `unknown function '${call.name}'`, call.pos);
      }
      if (call.args.length < fn.min || call.args.length > fn.max) {
        throw new ScriptError(
          'E_ARITY',
          `${call.name} expects ${fn.min}..${fn.max} arguments, got ${call.args.length}`,
          call.pos,
        );
      }
      if (usedAsExpression && fn.ret === 'none') {
        throw new ScriptError('E_NO_RETURN', `${call.name} does not return a value`, call.pos);
      }
      for (const arg of call.args) this.validateExpr(arg, true);
      return;
    }

    switch (call.name) {
      case 'log':
        if (usedAsExpression) {
          throw new ScriptError('E_NOT_EXPRESSION', 'log() is a statement, not an expression', call.pos);
        }
        if (call.args.length > 8) {
          throw new ScriptError('E_ARITY', 'log() accepts at most 8 arguments', call.pos);
        }
        for (const arg of call.args) this.validateExpr(arg, true);
        return;
      case 'wait': {
        if (usedAsExpression) {
          throw new ScriptError('E_NOT_EXPRESSION', 'wait() is a statement, not an expression', call.pos);
        }
        this.waitMilliseconds(call);
        return;
      }
      case 'call': {
        if (usedAsExpression) {
          throw new ScriptError('E_NOT_EXPRESSION', 'call() is a statement, not an expression', call.pos);
        }
        if (call.args.length < 1 || call.args.length > 5) {
          throw new ScriptError('E_ARITY', 'call() expects 1..5 arguments', call.pos);
        }
        this.scriptName(call.args[0], call.pos);
        for (const arg of call.args.slice(1)) this.validateExpr(arg, true);
        return;
      }
      case 'set':
        if (usedAsExpression) {
          throw new ScriptError('E_NOT_EXPRESSION', 'set() is a statement, not an expression', call.pos);
        }
        if (call.args.length !== 3) {
          throw new ScriptError('E_ARITY', 'set() expects 3 arguments', call.pos);
        }
        if (!this.isArrayExpression(call.args[0])) {
          throw new ScriptError('E_ARRAY_EXPR', 'set() first argument must be an array slot', call.args[0].pos);
        }
        for (const arg of call.args) this.validateExpr(arg, true);
        return;
      case 'get':
        if (call.args.length !== 2) {
          throw new ScriptError('E_ARITY', 'get() expects 2 arguments', call.pos);
        }
        if (!this.isArrayExpression(call.args[0])) {
          throw new ScriptError('E_ARRAY_EXPR', 'get() first argument must be an array expression', call.args[0].pos);
        }
        for (const arg of call.args) this.validateExpr(arg, true);
        return;
      case 'len':
        if (call.args.length !== 1) {
          throw new ScriptError('E_ARITY', 'len() expects 1 argument', call.pos);
        }
        this.validateExpr(call.args[0], true);
        return;
      case 'chr':
        if (call.args.length !== 1) {
          throw new ScriptError('E_ARITY', 'chr() expects 1 argument', call.pos);
        }
        this.validateExpr(call.args[0], true);
        return;
      case 'ord':
        if (call.args.length !== 1) {
          throw new ScriptError('E_ARITY', 'ord() expects 1 argument', call.pos);
        }
        this.validateExpr(call.args[0], true);
        return;
      default:
        throw new ScriptError('E_UNKNOWN_FUNCTION', `unknown function '${call.name}'`, call.pos);
    }
  }

  private validateNumber(expr: NumberExpr): void {
    if (!Number.isFinite(expr.value)) {
      throw new ScriptError('E_NUMBER_RANGE', 'number is not finite', expr.pos);
    }
    if (expr.isFloat) {
      if (Math.abs(expr.value) > 3.4028235e38) {
        throw new ScriptError('E_NUMBER_RANGE', 'float is outside float32 range', expr.pos);
      }
      return;
    }
    if (!Number.isInteger(expr.value) || expr.value > 0xffffffff || expr.value < -0x80000000) {
      throw new ScriptError('E_NUMBER_RANGE', 'integer is outside int32/uint32 range', expr.pos);
    }
  }

  private validateArrayLiteral(expr: ArrayExpr): void {
    if (expr.items.length > this.manifest.maxArrayLen) {
      throw new ScriptError(
        'E_ARRAY_LEN',
        `array literal exceeds ${this.manifest.maxArrayLen} items`,
        expr.pos,
      );
    }
    for (const item of expr.items) {
      if (!isNumericLiteral(item)) {
        throw new ScriptError('E_ARRAY_LITERAL', 'array items must be integer literals', item.pos);
      }
      if (item.value < 0 || item.value > 255) {
        throw new ScriptError('E_ARRAY_LITERAL', 'array item must be 0..255', item.pos);
      }
    }
  }

  private validateSlot(slot: VarSlot, write: boolean, pos: SourcePos): void {
    const max = this.manifest.vars[slot.type];
    if (slot.index < 0 || slot.index >= max) {
      throw new ScriptError(
        'E_SLOT_RANGE',
        `$${slot.type}${slot.index} is outside 0..${max - 1}`,
        pos,
      );
    }
    if (write && (slot.type === 'e' || slot.type === 'c')) {
      throw new ScriptError('E_READONLY', `$${slot.type}${slot.index} is read-only`, pos);
    }
  }

  private isArrayExpression(expr: Expr): boolean {
    if (expr.kind === 'var' && expr.slot?.type === 'a') return true;
    if (expr.kind === 'array') return true;
    return false;
  }

  private findFunction(name: string): FunctionSpec | undefined {
    return this.manifest.functions.find((fn) => fn.name === name);
  }

  private findEvent(name: string) {
    return this.manifest.events.find((event) => event.name === name);
  }

  private dataVar(name: string): DataVarSpec | undefined {
    return this.manifest.dataVars?.find((item) => item.name === name);
  }

  private waitMilliseconds(call: CallExpr): number {
    if (call.args.length !== 2) {
      throw new ScriptError('E_WAIT', 'wait() expects a number and a unit', call.pos);
    }
    const [valueArg, unitArg] = call.args;
    if (valueArg.kind !== 'number' || valueArg.isFloat || !Number.isInteger(valueArg.value)) {
      throw new ScriptError('E_WAIT', 'wait() value must be an integer literal', valueArg.pos);
    }
    if (unitArg.kind !== 'string') {
      throw new ScriptError('E_WAIT', 'wait() unit must be a literal', unitArg.pos);
    }
    const multiplier =
      unitArg.value === 'ms' ? 1 : unitArg.value === 's' ? 1000 : unitArg.value === 'm' ? 60000 : unitArg.value === 'h' ? 3600000 : 0;
    if (multiplier === 0) {
      throw new ScriptError('E_WAIT', `invalid wait unit '${unitArg.value}'`, unitArg.pos);
    }
    const ms = valueArg.value * multiplier;
    if (ms < 0 || ms > 0xffffffff) {
      throw new ScriptError('E_WAIT', 'wait duration is out of range', call.pos);
    }
    return ms;
  }

  private scriptName(expr: Expr, pos: SourcePos): string {
    if (expr.kind !== 'string') {
      throw new ScriptError('E_CALL_NAME', 'call() name must be a string literal', pos);
    }
    const name = expr.value.toLowerCase();
    if (!/^[a-z_][a-z0-9_-]{0,15}$/.test(name)) {
      throw new ScriptError(
        'E_CALL_NAME',
        'call() name must match [a-z_][a-z0-9_-]{0,15}',
        expr.pos,
      );
    }
    return name;
  }

  // ---------------------------------------------------------------------------
  // Code generation

  private genProgram(program: Program): void {
    const handlers = program.body.filter((stmt): stmt is OnStmt => stmt.kind === 'on');
    const main = program.body.filter((stmt) => stmt.kind !== 'on');
    for (const stmt of main) this.genStmt(stmt);
    this.emitOp('HALT');

    for (const handler of handlers) {
      const event = this.findEvent(handler.event);
      if (!event) {
        throw new ScriptError('E_UNKNOWN_EVENT', `unknown event '${handler.event}'`, handler.pos);
      }
      const start = this.code.length;
      for (const stmt of handler.body) this.genStmt(stmt);
      this.emitOp('RETURN');
      const end = this.code.length;
      this.blocks.push({ kind: BLOCK_HANDLER, start, middle: 0, end });
      this.handlers.push({
        eventId: event.id,
        nameConst: this.constants.addName(handler.event),
        entry: start,
        end,
      });
    }
  }

  private genStmt(stmt: Stmt): void {
    switch (stmt.kind) {
      case 'assign':
        this.genAssign(stmt.target, stmt.value);
        break;
      case 'expr': {
        const before = this.stackDepth;
        this.genExpr(stmt.expr, KIND_INT);
        if (this.stackDepth > before) this.emitOp('POP');
        break;
      }
      case 'if':
        this.genIf(stmt);
        break;
      case 'while':
        this.genWhile(stmt);
        break;
      case 'return':
        this.emitOp('RETURN');
        break;
      case 'break': {
        const loop = this.loopStack[this.loopStack.length - 1];
        if (!loop) throw new ScriptError('E_LOOP_SCOPE', 'break outside loop', stmt.pos);
        this.emitJump('JUMP', loop.breakLabel);
        break;
      }
      case 'continue': {
        const loop = this.loopStack[this.loopStack.length - 1];
        if (!loop) throw new ScriptError('E_LOOP_SCOPE', 'continue outside loop', stmt.pos);
        this.emitJump('JUMP', loop.continueLabel);
        break;
      }
      case 'on':
        // Handlers are compiled after the main program.
        return;
    }
    if (this.stackDepth !== 0) {
      throw new ScriptError('E_STACK_LEAK', `statement left ${this.stackDepth} values on the stack`, stmt.pos);
    }
  }

  private genAssign(target: LValue, value: Expr): void {
    if (target.kind === 'var') {
      if (!target.slot) throw new ScriptError('E_LVALUE', 'invalid variable target', target.pos);
      this.genExpr(value, this.lvalueKind(target));
      if (this.isWidePort(target.slot)) {
        this.emitOp('STORE_PORT', target.slot.index);
      } else {
        this.emitOp('STORE_VAR', target.slot.encoded);
      }
      return;
    }
    if (target.kind === 'data') {
      this.genExpr(value, this.dataVarKind(target.name, KIND_INT));
      this.emitOp('STORE_DATA', this.constants.addName(target.name));
      return;
    }
    this.genExpr(target.array, KIND_INT);
    this.genExpr(target.index, KIND_INT);
    this.genExpr(value, KIND_INT);
    this.emitOp('ARRAY_SET');
  }

  private genIf(stmt: Extract<Stmt, { kind: 'if' }>): void {
    const condStart = this.code.length;
    this.genExpr(stmt.cond, KIND_INT);
    const jumpOffset = this.code.length;
    const elseLabel = this.newLabel('else');
    this.emitJump('JUMP_IF_FALSE', elseLabel);
    for (const child of stmt.then) this.genStmt(child);
    const thenEnd = this.code.length;

    if (stmt.else && stmt.else.length > 0) {
      const endLabel = this.newLabel('endif');
      this.emitJump('JUMP', endLabel);
      this.bind(elseLabel);
      const elseStart = this.code.length;
      for (const child of stmt.else) this.genStmt(child);
      const end = this.code.length;
      this.bind(endLabel);
      this.blocks.push({ kind: BLOCK_IF, start: condStart, middle: jumpOffset, end: thenEnd });
      this.blocks.push({ kind: BLOCK_ELSE, start: elseStart, middle: 0, end });
    } else {
      this.bind(elseLabel);
      const end = this.code.length;
      this.blocks.push({ kind: BLOCK_IF, start: condStart, middle: jumpOffset, end });
    }
  }

  private genWhile(stmt: Extract<Stmt, { kind: 'while' }>): void {
    const condLabel = this.newLabel('while_cond');
    const breakLabel = this.newLabel('while_end');
    this.bind(condLabel);
    const condStart = this.code.length;
    this.genExpr(stmt.cond, KIND_INT);
    const jumpOffset = this.code.length;
    this.emitJump('JUMP_IF_FALSE', breakLabel);
    this.loopStack.push({ continueLabel: condLabel, breakLabel });
    for (const child of stmt.body) this.genStmt(child);
    this.loopStack.pop();
    this.emitJump('JUMP', condLabel);
    const end = this.code.length;
    this.bind(breakLabel);
    this.blocks.push({ kind: BLOCK_WHILE, start: condStart, middle: jumpOffset, end });
  }

  private genExpr(expr: Expr, expectedKind: number): void {
    switch (expr.kind) {
      case 'number':
        this.genNumber(expr);
        return;
      case 'string':
        this.emitOp('PUSH_STR', this.constants.addString(expr.value));
        return;
      case 'bool':
        this.emitOp('PUSH_U8', expr.value ? 1 : 0);
        return;
      case 'var':
        if (!expr.slot) {
          throw new ScriptError('E_LVALUE', 'variable has no slot', expr.pos);
        }
        if (this.isWidePort(expr.slot)) {
          this.emitOp('LOAD_PORT', expr.slot.index);
        } else {
          this.emitOp('LOAD_VAR', expr.slot.encoded);
        }
        return;
      case 'dataVar':
        this.emitOp(
          'LOAD_DATA',
          this.constants.addName(expr.name),
          expr.dataKind ?? this.dataVarKind(expr.name, expectedKind),
        );
        return;
      case 'unary': {
        const kind = expr.op === '-' ? this.unaryKind(expr, expectedKind) : KIND_INT;
        this.genExpr(expr.expr, kind);
        if (expr.op === '-') this.emitOp('NEG');
        else if (expr.op === '!') this.emitOp('NOT');
        else this.emitOp('BIT_NOT');
        return;
      }
      case 'binary': {
        const kind = this.binaryKind(expr, expectedKind);
        this.genExpr(expr.left, kind);
        this.genExpr(expr.right, kind);
        this.emitOp(this.binaryOpcode(expr.op));
        return;
      }
      case 'ternary':
        this.genTernary(expr, expectedKind);
        return;
      case 'call':
        this.genCall(expr);
        return;
      case 'index':
        this.genExpr(expr.array, KIND_INT);
        this.genExpr(expr.index, KIND_INT);
        this.emitOp('ARRAY_GET');
        return;
      case 'array':
        this.emitOp('PUSH_ARRAY', this.constants.addArray(this.arrayBytes(expr)));
        return;
    }
  }

  private genNumber(expr: NumberExpr): void {
    if (expr.isFloat) {
      this.emitOp('PUSH_F32', expr.value);
      return;
    }
    const value = expr.value;
    if (value >= 0) {
      if (value <= 0xff) this.emitOp('PUSH_U8', value);
      else if (value <= 0xffff) this.emitOp('PUSH_U16', value);
      else this.emitOp('PUSH_U32', value);
      return;
    }
    if (value >= -128) this.emitOp('PUSH_I8', value);
    else if (value >= -32768) this.emitOp('PUSH_I16', value);
    else this.emitOp('PUSH_I32', value);
  }

  private genTernary(expr: Extract<Expr, { kind: 'ternary' }>, expectedKind: number): void {
    const base = this.stackDepth;
    const condStart = this.code.length;
    this.genExpr(expr.cond, KIND_INT);
    const jumpOffset = this.code.length;
    const elseLabel = this.newLabel('tern_else');
    this.emitJump('JUMP_IF_FALSE', elseLabel);

    this.genExpr(expr.then, expectedKind);
    const thenEnd = this.code.length;
    const endLabel = this.newLabel('tern_end');
    this.emitJump('JUMP', endLabel);

    this.bind(elseLabel);
    this.stackDepth = base;
    const elseStart = this.code.length;
    this.genExpr(expr.else, expectedKind);
    const end = this.code.length;
    this.bind(endLabel);
    this.stackDepth = base + 1;
    if (this.stackDepth > this.stackMax) this.stackMax = this.stackDepth;

    this.blocks.push({ kind: BLOCK_TERNARY, start: condStart, middle: jumpOffset, end: thenEnd });
    this.blocks.push({ kind: BLOCK_ELSE, start: elseStart, middle: 0, end });
  }

  private genCall(call: CallExpr): void {
    if (!BUILTIN_NAMES.has(call.name)) {
      const fn = this.findFunction(call.name);
      if (!fn) throw new ScriptError('E_UNKNOWN_FUNCTION', `unknown function '${call.name}'`, call.pos);
      for (const arg of call.args) this.genExpr(arg, this.argKind(arg));
      this.emitDynamic('CALL_EXT', [fn.id, call.args.length], call.args.length, fn.ret === 'none' ? 0 : 1);
      return;
    }

    switch (call.name) {
      case 'log': {
        for (const arg of call.args) this.genExpr(arg, this.argKind(arg));
        this.emitDynamic('LOG', [call.args.length], call.args.length, 0);
        return;
      }
      case 'wait': {
        const ms = this.waitMilliseconds(call);
        this.emitOp('WAIT', ms);
        return;
      }
      case 'call': {
        const name = this.scriptName(call.args[0], call.pos);
        const args = call.args.slice(1);
        for (const arg of args) this.genExpr(arg, KIND_INT);
        this.emitDynamic(
          'CALL_SCRIPT_NAME',
          [this.constants.addName(name), args.length],
          args.length,
          0,
        );
        return;
      }
      case 'set': {
        for (const arg of call.args) this.genExpr(arg, KIND_INT);
        this.emitOp('SET');
        return;
      }
      case 'get':
        this.genExpr(call.args[0], KIND_INT);
        this.genExpr(call.args[1], KIND_INT);
        this.emitOp('GET');
        return;
      case 'len':
        this.genExpr(call.args[0], this.argKind(call.args[0]));
        this.emitOp('LEN');
        return;
      case 'chr':
        this.genExpr(call.args[0], KIND_INT);
        this.emitOp('CHR');
        return;
      case 'ord':
        this.genExpr(call.args[0], this.argKind(call.args[0]));
        this.emitOp('ORD');
        return;
      default:
        throw new ScriptError('E_UNKNOWN_FUNCTION', `unknown function '${call.name}'`, call.pos);
    }
  }

  private arrayBytes(expr: ArrayExpr): Uint8Array {
    const bytes = new Uint8Array(expr.items.length);
    expr.items.forEach((item, index) => {
      if (item.kind !== 'number' || item.isFloat) {
        throw new ScriptError('E_ARRAY_LITERAL', 'array items must be integer literals', item.pos);
      }
      bytes[index] = item.value & 0xff;
    });
    return bytes;
  }

  private isWidePort(slot: VarSlot): boolean {
    return slot.type === 'p' && slot.index > 15;
  }

  private lvalueKind(target: Extract<LValue, { kind: 'var' }>): number {
    const slot = target.slot;
    if (!slot) return KIND_INT;
    if (slot.type === 's') return KIND_STRING;
    if (slot.type === 'f') return KIND_FLOAT;
    if (slot.type === 'u') return KIND_UINT;
    return KIND_INT;
  }

  private dataVarKind(name: string, fallback: number): number {
    const spec = this.dataVar(name);
    return spec ? dataKindNumber(spec.kind) : fallback;
  }

  private argKind(expr: Expr): number {
    if (expr.kind === 'dataVar') return expr.dataKind ?? this.dataVarKind(expr.name, KIND_INT);
    if (this.isStringExpr(expr)) return KIND_STRING;
    if (this.isFloatExpr(expr)) return KIND_FLOAT;
    return KIND_INT;
  }

  private unaryKind(expr: Extract<Expr, { kind: 'unary' }>, fallback: number): number {
    if (this.isFloatExpr(expr.expr)) return KIND_FLOAT;
    return fallback;
  }

  private binaryKind(expr: Extract<Expr, { kind: 'binary' }>, fallback: number): number {
    if (expr.op === '&&' || expr.op === '||') return KIND_INT;
    if (this.isStringExpr(expr.left) || this.isStringExpr(expr.right)) return KIND_STRING;
    if (this.isFloatExpr(expr.left) || this.isFloatExpr(expr.right)) return KIND_FLOAT;
    return fallback;
  }

  private isStringExpr(expr: Expr): boolean {
    if (expr.kind === 'string') return true;
    if (expr.kind === 'var') return expr.slot?.type === 's';
    if (expr.kind === 'dataVar') {
      return expr.dataKind === KIND_STRING || this.dataVar(expr.name)?.kind === 'string';
    }
    if (expr.kind === 'call') return expr.name === 'chr';
    if (expr.kind === 'binary' && expr.op === '+') {
      return this.isStringExpr(expr.left) || this.isStringExpr(expr.right);
    }
    if (expr.kind === 'ternary') {
      return this.isStringExpr(expr.then) || this.isStringExpr(expr.else);
    }
    return false;
  }

  private isFloatExpr(expr: Expr): boolean {
    if (expr.kind === 'number') return expr.isFloat;
    if (expr.kind === 'var') return expr.slot?.type === 'f';
    if (expr.kind === 'dataVar') {
      return expr.dataKind === KIND_FLOAT || this.dataVar(expr.name)?.kind === 'float';
    }
    if (expr.kind === 'unary') return this.isFloatExpr(expr.expr);
    if (expr.kind === 'binary') {
      if (expr.op === '&&' || expr.op === '||') return false;
      return this.isFloatExpr(expr.left) || this.isFloatExpr(expr.right);
    }
    if (expr.kind === 'ternary') {
      return this.isFloatExpr(expr.then) || this.isFloatExpr(expr.else);
    }
    if (expr.kind === 'call') {
      const fn = this.findFunction(expr.name);
      return fn?.ret === 'float';
    }
    return false;
  }

  private binaryOpcode(op: BinaryOp): OpcodeName {
    switch (op) {
      case '+':
        return 'ADD';
      case '-':
        return 'SUB';
      case '*':
        return 'MUL';
      case '/':
        return 'DIV';
      case '%':
        return 'MOD';
      case '<<':
        return 'SHL';
      case '>>':
        return 'SHR';
      case '<':
        return 'LT';
      case '<=':
        return 'LE';
      case '>':
        return 'GT';
      case '>=':
        return 'GE';
      case '==':
        return 'EQ';
      case '!=':
        return 'NE';
      case '&':
        return 'BIT_AND';
      case '^':
        return 'BIT_XOR';
      case '|':
        return 'BIT_OR';
      case '&&':
        return 'AND';
      case '||':
        return 'OR';
    }
  }

  // ---------------------------------------------------------------------------
  // Emission helpers

  private newLabel(prefix: string): string {
    return `${prefix}_${this.labelCounter++}`;
  }

  private bind(label: string): void {
    this.labels.set(label, this.code.length);
  }

  private emitJump(op: 'JUMP' | 'JUMP_IF_FALSE' | 'JUMP_IF_TRUE', label: string): void {
    this.writeOpcode(op);
    const at = this.code.length;
    writeU16(this.code, 0);
    this.fixups.push({ at, label });
    const info = OPCODES[op];
    this.popValue(info.pop);
    this.pushValue(info.push);
  }

  private emitOp(op: OpcodeName, ...operands: number[]): void {
    const info = OPCODES[op];
    this.writeOpcode(op);
    for (let i = 0; i < info.operands.length; i++) {
      const type = info.operands[i];
      const value = operands[i] ?? 0;
      const size = operandSize(type);
      if (type === 'f32') writeF32(this.code, value);
      else if (size === 1) this.code.push(value & 0xff);
      else if (size === 2) writeU16(this.code, value);
      else writeU32(this.code, value);
    }
    this.popValue(info.pop);
    this.pushValue(info.push);
  }

  private emitDynamic(op: OpcodeName, operands: number[], pop: number, push: number): void {
    const info = OPCODES[op];
    this.writeOpcode(op);
    for (let i = 0; i < info.operands.length; i++) {
      const type = info.operands[i];
      const value = operands[i] ?? 0;
      const size = operandSize(type);
      if (type === 'f32') writeF32(this.code, value);
      else if (size === 1) this.code.push(value & 0xff);
      else if (size === 2) writeU16(this.code, value);
      else writeU32(this.code, value);
    }
    this.popValue(pop);
    this.pushValue(push);
  }

  private writeOpcode(op: OpcodeName): void {
    this.code.push(Opcode[op]);
  }

  private pushValue(count: number): void {
    this.stackDepth += count;
    if (this.stackDepth > this.stackMax) this.stackMax = this.stackDepth;
  }

  private popValue(count: number): void {
    this.stackDepth -= count;
    if (this.stackDepth < 0) {
      throw new ScriptError('E_STACK', 'stack underflow while compiling', { line: 1, column: 1, offset: 0 });
    }
  }

  private patchFixups(): void {
    for (const fixup of this.fixups) {
      const target = this.labels.get(fixup.label);
      if (target === undefined) {
        throw new ScriptError('E_LABEL', `unbound label '${fixup.label}'`, { line: 1, column: 1, offset: 0 });
      }
      if (target > 0xffff) {
        throw new ScriptError('E_CODE_SIZE', 'jump target exceeds 16 bits', { line: 1, column: 1, offset: 0 });
      }
      this.code[fixup.at] = target & 0xff;
      this.code[fixup.at + 1] = (target >>> 8) & 0xff;
    }
  }
}

export function compile(source: string, options: CompileOptions = {}): CompileResult {
  const warnings: CompileWarning[] = [];
  try {
    const program = parse(source);
    const canonical = printProgram(program);
    const compiler = new BytecodeCompiler(options.manifest ?? DEFAULT_MANIFEST, warnings);
    const result = compiler.compile(program, canonical);
    return {
      ok: true,
      errors: [],
      warnings,
      bytecode: result.bytecode,
      disassembly: disassemble(result.bytecode),
      source: canonical,
      stats: result.stats,
    };
  } catch (err) {
    if (err instanceof ScriptError) {
      return {
        ok: false,
        errors: [{ code: err.code, message: err.message, pos: err.pos }],
        warnings,
      };
    }
    if (err instanceof ContainerError) {
      return {
        ok: false,
        errors: [{ code: err.code, message: err.message, pos: { line: 1, column: 1, offset: 0 } }],
        warnings,
      };
    }
    throw err;
  }
}
