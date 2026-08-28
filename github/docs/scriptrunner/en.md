# ScriptRunner — Syntax and User Guide

Version: 1.3.0  
Updated: 2026-08-28


## Introduction

ScriptRunner is a script interpreter for ESP8266/ESP32 with cooperative multitasking. Scripts allow controlling ports, variables, events, and external functions.


## Basic Rules

### Command Separator

All commands are separated by a semicolon `;`.

    $v0=0;while:$v0<10;log($v0);$v0=$v0+1;wait(1s);end;

### Whitespace

Whitespace is allowed inside expressions (on the right side of assignments, in function arguments, in the ternary operator). It is ignored by the parser.

**Where you can put spaces:**
- Around operators: `$v0 = 10 + 5`
- In the ternary operator: `$v0 > 5 ? 100 : 0`
- In function parameters: `log('Hello', $v0)`

**Where spaces are NOT allowed:**
- Between commands (commands are separated only by `;`).
- Inside keywords: `if:`, `while:`, `on(`, `end`, `else`.
- In time suffixes: `100ms`, `1s` — no space.

**Recommendation:** for brevity and reliability, it is better not to use spaces in scripts.

### Strings

Only single quotes `'` are allowed.

    $s0='Hello World';
    log('Temperature:',$temp);
    on('btn');

### Comments

Comments are not supported.


## Variables

| Type | Syntax | Example |
|------|--------|---------|
| Unsigned integer | `$vN` | `$v0=255` |
| Signed integer | `$iN` | `$i0=-10` |
| Floating point | `$fN` | `$f0=3.14` |
| String | `$sN` | `$s0='Hello'` |
| Array | `$aN` | `$a0={1,2,3}` |
| Port (pin) | `$pN` | `$p14=255` |

`N` — variable number from 0 to 9.


## Delays (wait)

    wait(10ms);   // 10 milliseconds
    wait(100ms);  // 100 milliseconds
    wait(500ms);  // 500 milliseconds
    wait(1s);     // 1 second
    wait(5s);     // 5 seconds
    wait(1m);     // 1 minute
    wait(5m);     // 5 minutes
    wait(1h);     // 1 hour
    wait(1);      // 1 second (compatibility)

**Important:**
- Minimum real delay ~10 ms
- Only integers
- No spaces between number and suffix


## Working with Ports

### Write Value

    $p2=255;      // Turn on (PWM)
    $p14=128;     // Half brightness (PWM)
    $p0=0;        // Turn off

### Fade (Smooth Change)

    fade(14,255,2000);  // Fade to 255 over 2 seconds
    fade(13,0,1000);    // Fade to 0 over 1 second

Format: `fade(gpio, value, time)`  
- `gpio` — pin number  
- `value` — target value (0–255)  
- `time` — duration in milliseconds

### Read Value

    $v0=$p0;      // Read pin 0 into variable
    log('Pin0:',$p0);

### Limitations

- INPUT ports (buttons): read-only, generate events
- OUTPUT ports (LEDs): read and write


## Events

### Event Formats for Buttons

Events are automatically generated when the state of INPUT ports changes:

- `btn` — any change
- `btn_c` — click
- `btn_l` — long press (1–2 seconds)
- `btn_r` — hold > 2 seconds (repeats every 100 ms)

### Event Handler

    on('btn');
      $p2=255;
      wait(200ms);
      $p2=0;
    end;


## Control Structures

### While Loop

    $v0=0;
    while:$v0<10;
      log($v0);
      $v0=$v0+1;
      wait(1s);
    end;

### If / Else

    $v0=5;
    if:$v0>10;
      log('Greater');
    else;
      log('Less or equal');
    end;

### Complex Conditions

    if:$v0>=0&&$v0<=10;
      log('In range');
    end;

    if:$v0==10||$v0==20;
      log('Ten or twenty');
    end;


## Logging (log)

    log('Hello');
    log('Count:',$v0);
    log('Pin0=',$p0,'Pin1=',$p1);
    log($v0,$v1,$v2);
    log('Temp:',$temp,'Hum:',$hum);

**Features:**
- Supports multiple parameters separated by commas
- Automatically adds a space between parameters
- Strings only in single quotes


## Expressions and Operators

Scripts support full expressions with operators, parentheses, and the ternary conditional operator.

### Arithmetic Operators

    $v0=10+5;
    $v1=10-3;
    $v2=10*3;
    $v3=10/3;
    $v4=10%3;    // remainder

### Bitwise Operators

    $v4=10&3;     // AND
    $v5=10|3;     // OR
    $v6=10^3;     // XOR
    $v7=10<<2;    // Shift left
    $v8=10>>1;    // Shift right

### Comparison Operators

Available in expressions (not only in `if`/`while`):

    $v0=5>3?1:0;          // 1
    $v1=10==10?100:0;     // 100
    $v2=$v0!=0?$v0:0;     // 1
    $v3=(2+3)*4>10?1:0;   // 1

Full list: `==`, `!=`, `<`, `>`, `<=`, `>=`

### Ternary Operator (`? :`)

Conditional operator that selects a value based on a condition.  
Works **only in expressions** (on the right side of assignments, in function arguments, in `log`).  
**Does NOT work** in `if` and `while` conditions.

**Syntax:**

    condition ? value_if_true : value_if_false

**Examples:**

    $v0=10>5?100:0;            // $v0=100
    $v1=$v0>50?$v0*2:$v0/2;
    $s0=$v0==0?'zero':'non-zero';
    $p14=$v0>100?255:0;
    $f0=3.14;
    $f1=$f0>3.0?6.28:1.57;

    // Nesting with parentheses (supported)
    $v0=5;
    $v1=$v0>0?($v0<10?1:2):0;   // $v1=1

    // Grouping complex expressions
    $v2=(($v0+$v1)*2)>10?100:0;

**Parentheses** `( )` are supported for explicit precedence.

### Unary Negation `!`

Logical NOT. Applies to a number, variable, string, or array. Result is an integer `1` (true) or `0` (false).

**Examples:**

    $v0=0;
    $v1=!$v0;          // $v1=1
    $s0='';
    $v2=!$s0;          // $v2=1 (empty string → false → inversion)
    $v3=!5;            // 5 → true → inversion → 0
    $v4=!$v0?100:55;   // $v4=100

**Operator precedence (left to right):**
1. Unary: `-`, `~`, `!`
2. Arithmetic: `*`, `/`, `%`, `+`, `-`
3. Bitwise: `&`, `^`, `|`
4. Shift: `<<`, `>>`
5. Comparison: `==`, `!=`, `<`, `>`, `<=`, `>=`
6. Ternary: `? :`

**Important:** precedence is evaluated strictly left to right. Use parentheses to change the order.


## Working with Arrays

    // Initialization
    $a0={1,2,3,4,5};

    // Array length
    $v0=len($a0);

    // Read element
    $v1=get($a0,0);

    // Write element
    set($a0,2,10);

    // Fill
    $v0=0;
    while:$v0<5;
      set($a0,$v0,$v0*10);
      $v0=$v0+1;
    end;


## Working with Strings

    // Concatenation
    $s0='Hello ';
    $s1='World';
    $s2=$s0+$s1;

    // String length
    $v0=len($s0);

    // Code of first character
    $v1=ord($s0);    // 'H' -> 72

    // Character by code
    $s3=chr(65);     // 65 -> 'A'


## External Functions and Variables

    $temp=udp(2);
    $hum=fade(2,255,1000);

    log('Temp:',$temp);
    if:$temp>30;
      log('Too hot!');
    end;


## Example Scripts

### Blink LED

    $p2=255;wait(500ms);$p2=0;wait(500ms);

### Counter

    $v0=0;while:$v0<10;log('Count:',$v0);$v0=$v0+1;wait(1s);end;log('Done!');

### Smooth Fade

    $v0=0;
    while:$v0<5;
      fade(14,255,1000);
      wait(1s);
      fade(14,0,1000);
      wait(1s);
      $v0=$v0+1;
    end;

### Button Toggles LED

    $state=0;
    on('btn_click');
      if:$state==0;
        $state=1;
        $p2=255;
        log('ON');
      else;
        $state=0;
        $p2=0;
        log('OFF');
      end;
    end;


## Limitations

- Maximum script length: 256 characters
- Maximum simultaneous scripts: 15
- Minimum delay: ~10 ms
- No spaces outside expressions (spaces allowed inside expressions)
- Only single quotes `'`
- No fractional numbers in `wait()`
- No comments


## Quick Reference

| Construct | Example |
|-----------|---------|
| Separator | `;` |
| Strings | `'text'` |
| Spaces | allowed inside expressions |
| Variables | `$v0`, `$i0`, `$f0`, `$s0`, `$a0` |
| Delay | `wait(100ms)`, `wait(1s)`, `wait(1m)`, `wait(1h)` |
| Loop | `while:condition; ... end;` |
| Condition | `if:condition; ... else; ... end;` |
| Event | `on('name'); ... end;` |
| Log | `log(param1, param2, ...)` |
| Ports | `$p13` |
| Fade | `fade(gpio, value, time)` |
| Ternary operator | `condition?true:false` |
| Parentheses in expressions | `($v0+$v1)*2` |
| Unary negation | `!$v0` |