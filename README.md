# Vuesp 

Vue + ESP = Vuesp

Template for your projects with a web interface.

![image](https://raw.githubusercontent.com/bondrogeen/vuesp/master/github/images/screen1.png)


[ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer) - Async HTTP and WebSocket Server for ESP Arduino   
[Vue](https://vuejs.org/) - The Progressive
JavaScript Framework     
[AT-UI](https://at-ui.github.io/at-ui/#/en) - A Lightweight and Modular Front-End UI Library   


## Project structure

- /data/
- /github/
    - /images/
- /src/
    - main.cpp
- /vue/
    - /public/
    - /src/
    - vue.config.js
- platformio.ini


## Front vue dev
```
git clone https://github.com/bondrogeen/vuesp
cd vuesp/vue
npm i
```

Next in  [vue.config.js](./vue/vue.config.js) file change proxy to your ip device

```js
  devServer: {
    proxy: 'http://192.168.1.59/' // Change to your ip esp device
  }
```

#### Start dev server
```
npm run serve
```

http://localhost:8080/ 



## Build the project
```
npm run build
```

platformio run --target uploadfs --environment nodemcuv2


### 0.0.3 (2022-01-30)
* (bondrogeen) add read FS

### 0.0.2 (2022-01-15)
* (bondrogeen) update firmware

### 0.0.1 (2022-01-07)
* (bondrogeen) init

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
