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
    source: `on('btn_c', (gpio, value, count) => {
  log('button', gpio, value, count);
});
`,
  },
  {
    name: 'time',
    source: `on('time', (hours, minutes) => {
  log('time', hours, minutes);
});
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
  {
    name: 'button_toggle',
    source: `// Кнопка на GPIO4 включает/выключает реле на GPIO12.
$u0 = 0;
$p12 = 0;

on('btn_c', (gpio, value, count) => {
  $u0 = $u0 == 0 ? 1 : 0;
  $p12 = $u0;
  log('relay', gpio, $u0);
});
`,
  },
  {
    name: 'button_long',
    source: `// Клик — мигание GPIO13, долгое нажатие — сброс.
$u0 = 0;
$p13 = 0;

on('btn_c', (gpio, value, count) => {
  $u0 = $u0 + 1;
  $p13 = ($u0 & 1) ? 255 : 0;
  log('click', gpio, count, $u0);
});

on('btn_l', (gpio, value) => {
  $u0 = 0;
  $p13 = 0;
  log('long', gpio, value);
});
`,
  },
  {
    name: 'thermo_fan',
    source: `// LM35 на A0 (GPIO17): 10 мВ/°C, 3.3 В, 10 бит.
// Вентилятор/реле на GPIO13 с гистерезисом 27..30 °C.
$u0 = 0;
$f0 = 0.0;
$p13 = 0;

while (1) {
  $u0 = $p17;
  $f0 = $u0 * 330.0 / 1023.0;
  if ($f0 > 30.0) { $p13 = 255; }
  if ($f0 < 27.0) { $p13 = 0; }
  log('temp', $f0, $u0, $p13);
  wait(2s);
}
`,
  },
  {
    name: 'adc_alarm',
    source: `// Аналоговый датчик на A0 (GPIO17): реле на GPIO12 и лог.
$u0 = 0;
$p12 = 0;

while (1) {
  $u0 = $p17;
  if ($u0 > 700) {
    $p12 = 1;
  } else if ($u0 < 500) {
    $p12 = 0;
  }
  log('adc', $u0, $p12);
  wait(500ms);
}
`,
  },
  {
    name: 'pwm_dimmer',
    source: `// Клик по кнопке — яркость GPIO13 шагами по 64, долгое — 50%.
$u0 = 64;
$p13 = $u0;

on('btn_c', (gpio, value) => {
  $u0 = $u0 + 64;
  if ($u0 > 255) { $u0 = 0; }
  $p13 = $u0;
  log('brightness', gpio, $u0);
});

on('btn_l', (gpio, value) => {
  $u0 = 128;
  $p13 = $u0;
  log('reset', gpio, $u0);
});
`,
  },
  {
    name: 'heartbeat',
    source: `// Периодический «пульс»: мигание GPIO14 и лог каждую секунду.
$u0 = 0;

while (1) {
  $u0 = $u0 + 1;
  $p14 = $p14 > 0 ? 0 : 255;
  log('beat', $u0);
  wait(1s);
}
`,
  },
];
