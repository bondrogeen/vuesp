export interface BytecodeExample {
  readonly name: string;
  readonly source: string;
}

// Ready-to-compile sources for the bytecode studio. Names use the same
// lowercase format as the device script pack.
export const BYTECODE_EXAMPLES: readonly BytecodeExample[] = [
  {
    name: 'boot',
    source: `$display = 'boot';
log('boot start');
$p13 = 128;
wait(500ms);
$p13 = 0;
log('boot done');
`,
  },
  {
    name: 'blink',
    source: `$u0 = 0;
while ($u0 < 5) {
  $p13 = 255;
  wait(500ms);
  $p13 = 0;
  wait(500ms);
  $u0 = $u0 + 1;
}
`,
  },
  {
    name: 'button',
    source: `on('btn_c') {
  log('button', $e0, $e1, $e2);
}
`,
  },
  {
    name: 'counter',
    source: `$u0 = 1;
while ($u0 <= 5) {
  log('counter', $u0);
  $u0 = $u0 + 1;
  wait(1s);
}
log('done');
`,
  },
  {
    name: 'expressions',
    source: `$u0 = 7;
$u1 = 3;
log('sum', $u0 + $u1, 'mul', $u0 * $u1, 'mod', $u0 % $u1);
$s0 = 'Vuesp';
log('text', $s0, 'len', len($s0));
`,
  },
  {
    name: 'array',
    source: `$a0 = [1, 2, 3];
log('len', len($a0), 'first', get($a0, 0));
set($a0, 0, 10);
log('new', get($a0, 0));
`,
  },
  {
    name: 'display',
    source: `$display = 'BC-OK';
`,
  },
  {
    name: 'chain',
    source: `call('display');
call('blink');
`,
  },
];
