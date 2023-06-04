# Vuesp

Vue + ESP = Vuesp

Template for your projects with a web interface.

![image](https://raw.githubusercontent.com/bondrogeen/vuesp/master/github/images/screen.gif)

[ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer) - Async HTTP and WebSocket Server for ESP Arduino  
[Vue3](https://vuejs.org/) - The Progressive JavaScript Framework

## Project structure

- /data/
- /firmware/
  - /esp32/
    - firmware.bin
    - littlefs.bin
  - /esp8266/
    - firmware.bin
    - littlefs.bin
- /github/
  - /images/
- /src/
  - /const/
    - index.h
    - define.h
    - struct.h
  - /pages/
    - recovery.h
  - /include/
    - init.h
    - tasks.h
    - webserver.h
    - gpio.h
  - gpio.cpp
  - init.cpp
  - main.cpp
  - tasks.cpp
  - webserver.cpp
- /vue/
  - /public/
  - /src/
  - vue.config.js
- platformio.ini
- script.sh
- mklittlefs

## Development

```
git clone https://github.com/bondrogeen/vuesp
cd vuesp/vue
npm i
```

Next in [.env](./vue/.env) file change proxy to your ip device

```
PROXY=192.168.1.10 // Change to your ip esp device
```

```
npm run dev
```

http://localhost:3000/

## Build the project

```
npm run build
```

### 1.3.0 (2023-06-05)

- (bondrogeen) Add example

### 1.2.1 (2023-06-04)

- (bondrogeen) Minor fixes

### 1.2.0 (2023-05-28)

- (bondrogeen) Add GPIO

### 1.1.0 (2022-11-7)

- (bondrogeen) Changed lib struct
- (bondrogeen) Add structure initialization events

### 1.0.0 (2022-10-23)

- (bondrogeen) Changed project structure
- (bondrogeen) Changed web interface
- (bondrogeen) Added build scripts

### 0.1.0 (2022-10-16)

- (bondrogeen) Migrate from Vue2 to Vue3
- (bondrogeen) Changed project structure
- (bondrogeen) Adding a dark theme

### 0.0.3 (2022-01-30)

- (bondrogeen) add read FS

### 0.0.2 (2022-01-15)

- (bondrogeen) update firmware

### 0.0.1 (2022-01-07)

- (bondrogeen) init

## License

The MIT License (MIT)

Copyright (c) 2021-2022, bondrogeen <bondrogeen@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
