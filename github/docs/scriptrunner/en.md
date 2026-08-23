# ScriptRunner — Syntax and User Guide

Version: 1.0.0
Updated: 2026-07-15


## Introduction

ScriptRunner is a script interpreter for ESP8266/ESP32 with cooperative multitasking. Scripts allow you to control ports, variables, events, and external functions.


## Basic Rules

### Command Separator

All commands are separated by semicolon ;

```scriptrunner
$v0=0;while:$v0<10;log($v0);$v0=$v0+1;wait(1s);end;
```

### Spaces

Forbidden outside string literals.

Correct:
$v0=10;if:$v0>5;log('Greater');end;

Incorrect:
$v0 = 10; if : $v0 > 5; log('Greater'); end;

### Strings

Only single quotes '

```scriptrunner
$s0='Hello World';
log('Temperature:',$temp);
on('btn_13_1');
```

### Comments

Comments are not supported.


## Variables

Type                Syntax    Example
Unsigned integer    $vN       $v0=255
Signed integer      $iN       $i0=-10
Floating point      $fN       $f0=3.14
String              $sN       $s0='Hello'
Array               $aN       $a0={1,2,3}
Port (pin)          $pN       $p14=255

N — variable number from 0 to 9.


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

Important:
- Minimum real delay ~10ms
- Only integer numbers
- No spaces between number and suffix


## Working with Ports

### Writing Value

```scriptrunner
$p2=255;      // Turn on (PWM)
$p14=128;     // Half brightness (PWM)
$p0=0;        // Turn off
```

### Smooth Change (Fade)

```scriptrunner
$p14=255/20;  // Smooth to 255 over 2 seconds (PWM)
$p14=0/10;    // Smooth to 0 over 1 second (PWM)
```

Format: $pN=value/tenths_of_second
- tenths_of_second — number of 100ms steps
- 10 = 1 second, 20 = 2 seconds

### Reading Value

```scriptrunner
$v0=$p0;      // Read pin 0 into variable
log('Pin0:',$p0);
```

### Limitations

- INPUT ports (buttons): read-only, generate events
- OUTPUT ports (LEDs): read and write


## Events

### Button Event Format

Events are generated automatically when INPUT port state changes:

btn_<gpio>          // Any change on the port
btn_<gpio>_<value>  // Specific value (0 or 1)

### Event Handler

```scriptrunner
on('btn_13_1');
$p2=255;
wait(200ms);
$p2=0;
end;
```

### Button Event Examples

Event         Description
btn_13        Any change on pin 13
btn_13_1      Pin 13 became 1 (press)
btn_13_0      Pin 13 became 0 (release)
btn_2         Any change on pin 2
btn_2_1       Pin 2 became 1 (press)
btn_2_0       Pin 2 became 0 (release)


## Control Structures

### While Loop

```scriptrunner
$v0=0;
 while:$v0<10;
 log($v0);
 $v0=$v0+1;
 wait(1s);
end;
```

### If/Else
```scriptrunner
$v0=5;
if:$v0>10;
 log('Greater');
else;
 log('Less or equal');
end;
```

### Complex Conditions

```scriptrunner
if:$v0>=0&&$v0<=10;
 log('In range');
end;

if:$v0==10||$v0==20;
 log('Ten or twenty');
end;
```

### Nested Structures

```scriptrunner
$temp=read_temp();
if:$temp>30;
 log('Hot!');
 if:$temp>40;
  log('CRITICAL!');
  emit('overheat');
 end;
else;
 log('Normal');
end;
```

## Logging (log)

```scriptrunner
log('Hello');
log('Count:',$v0);
log('Pin0=',$p0,'Pin1=',$p1);
log($v0,$v1,$v2);
log('Temp:',$temp,'Hum:',$hum);
```

Features:
- Supports multiple parameters separated by commas
- Automatically adds a space between parameters
- Strings only in single quotes


## Math Operations

```scriptrunner
$v0=10+5;
$v1=10-3;
$v2=10*3;
$v3=10/3;

$v4=10&3;     // AND
$v5=10|3;     // OR
$v6=10^3;     // XOR
$v7=10<<2;    // Left shift
$v8=10>>1;    // Right shift
$v9=10%3;     // Modulo
```

## Working with Arrays

```scriptrunner
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
```


## Working with Strings

```scriptrunner
// Concatenation
$s0='Hello ';
$s1='World';
$s2=$s0+$s1;

// String length
$v0=len($s0);

// First character code
$v1=ord($s0);    // 'H' -> 72

// Character by code
$s3=chr(65);     // 65 -> 'A'
```

## External Functions and Variables

```scriptrunner
$temp=read_temp(2);
$hum=read_hum(2);
$motion=read_motion();

log('Temp:',$temp);
if:$temp>30;
 log('Too hot!');
end;
```

## Script Examples

### Blinking LED

```scriptrunner
$p2=255;wait(500ms);$p2=0;wait(500ms);
```

### Counter

```scriptrunner
$v0=0;while:$v0<10;log('Count:',$v0);$v0=$v0+1;wait(1s);end;log('Done!');
```

### Smooth Fade

```scriptrunner
$p14=255/20;wait(2s);$p14=0/20;
```

### Button Controls LED

```scriptrunner
$state=0;
on('btn_13_1');
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
```

### Smart Light with Two Buttons

```scriptrunner
$brightness=128;
$state=0;
on('btn_13_1');
 if:$state==0;
  $state=1;
  $p14=$brightness/10;
  log('ON');
 else;
  $state=0;
  $p14=0/20;
  log('OFF');
 end;
end;
```

```scriptrunner
on('btn_2_1');
 if:$state==1;
  $brightness=$brightness+32;
  if:$brightness>255;$brightness=255;end;
  $p14=$brightness/10;
  log('Brightness:',$brightness);
 end;
end;
```

## Limitations

- Maximum script length: 256 characters
- Maximum scripts simultaneously: 15
- Minimum delay: ~10ms
- No spaces outside strings
- Only single quotes '
- No fractional numbers in wait()
- No comments


## Quick Reference

Separator                   ;
Strings                     'text'
Spaces                      only inside strings
Variables                   $v0, $i0, $f0, $s0, $a0
Delay                       wait(100ms), wait(1s), wait(1m), wait(1h)
Loop                        while:condition; ... end;
Condition                   if:condition; ... else; ... end;
Event                       on('name'); ... end;
Log                         log(param1,param2,...);
Ports                       $p13
Smooth change (PWM)         $p14=255/20;