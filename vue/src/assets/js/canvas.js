const defColors = [
  [0, 0, 0, 255],
  [127, 127, 127, 255],
  [255, 0, 0, 255],
  [0, 255, 0, 255],
  [0, 0, 255, 255],
  [255, 242, 0, 255],
  [34, 177, 36, 255],
  [0, 162, 232, 255],
  [63, 72, 204, 255],
  [163, 73, 164, 255],
  [163, 73, 164, 255],
  [255, 255, 255, 255],
];

import { int32ToBytes } from '@/utils/helpers'

const rotateRight90 = (matrix) => {
  let result = [];
  for (let i = matrix.length - 1; i >= 0; i--) {
    for (let j = 0; j < matrix[i].length; j++) {
      if (!result[j]) result[j] = [];
      result[j].push(matrix[i][j]);
    }
  }
  return result;
}

const rotateRight180 = (matrix) => rotateRight90(rotateRight90(matrix))
const rotateRight270 = (matrix) => rotateRight90(rotateRight90(rotateRight90(matrix)))
export default class Canvas {
  constructor({ width, height, colors, event, color = [255, 255, 255, 255], fill = [0, 0, 0, 255], rotate = '180', direction = 'serpentine' }) {
    this.canvas = document.querySelector('canvas');
    this.width = +width;
    this.height = +height;
    this.tool = 'pen';
    this.rotate = rotate;
    this.direction = direction;
    this.fill = fill;
    this.event = event;
    this.colors = colors || defColors;
    this.canvas.width = 10 * this.width;
    this.canvas.height = 10 * this.height;
    this.canvas.style.display = 'block';
    this.w = +this.canvas.width;
    this.h = +this.canvas.height;
    this.ctx = this.canvas.getContext('2d');
    this.ctx.fillStyle = `rgba(${this.fill.join(',')})`;
    this.ctx.globalAlpha = 1;
    this.ctx.fillRect(0, 0, this.w, this.h);
    this.data = [...Array(this.height)].map(() => Array(this.width).fill(this.fill));
    this.steps = [];
    this.redo_arr = [];
    this.frames = [];
    this.color = color;

    this.canvas.addEventListener('click', e => {
      const { x, y } = this.getPosition(e)
      if (this.tool === 'pen') {
        this.draw(x, y);
      } else if (this.tool === 'eraser') {
        this.erase(x, y);
      }
    });



    this.canvas.addEventListener('contextmenu', e => {
      e.preventDefault();
      const { x, y } = this.getPosition(e)
      if (this.tool === 'eraser') {
        this.erase(x, y);
      }
    })

    this.canvas.addEventListener('mousemove', e => {
      if (this.active) {
        const { x, y } = this.getPosition(e)
        if (this.tool === 'pen') {
          this.draw(x, y);
        } else if (this.tool === 'eraser') {
          this.erase(x, y);
        }
      }
    });

    this.canvas.addEventListener('touchmove', e => {
      const { x, y } = this.getPosition(e)
      if (this.tool === 'pen') {
        this.draw(x, y);
      } else if (this.tool === 'eraser') {
        this.erase(x, y);
      }
    });

    this.canvas.addEventListener('mouseleave', () => {
      this.active = false;
    });

    this.canvas.addEventListener('mousedown', (e) => {
      this.active = true;
      this.setTool(e.button ? 'eraser' : 'pen')
    });

    this.canvas.addEventListener('mouseup', () => {
      this.active = false;
      if (this?.event?.click) this.event.click();
    });
  }

  getPosition(e) {
    const rect = this.canvas.getBoundingClientRect();
    let x = e.clientX - rect.left;
    let y = e.clientY - rect.top;
    x = Math.floor((this.width * x) / this.canvas.clientWidth);
    y = Math.floor((this.height * y) / this.canvas.clientHeight);
    return { x, y }
  }

  draw(x, y, count) {
    // console.log(x, y);
    if (x >= 0 && x < this.width && y >= 0 && y < this.height) {
      this.data[x][y] = this.color;
      this.ctx.fillRect(Math.floor(x * (this.w / this.width)), Math.floor(y * (this.h / this.height)), Math.floor(this.w / this.width), Math.floor(this.h / this.height));
      if (!count && JSON.stringify(this.steps[this.steps.length - 1]) !== JSON.stringify([x, y, this.color, this.ctx.globalAlpha])) this.steps.push([x, y, this.color, this.ctx.globalAlpha]);
    }
  }

  setTool(tool) {
    this.tool = tool;
  }

  getRotate() {
    let data = JSON.parse(JSON.stringify(this.data))

    if (this.rotate === '90') {
      data = rotateRight90(data)
    }
    if (this.rotate === '180') {
      data = rotateRight180(data)
    }
    if (this.rotate === '270') {
      data = rotateRight270(data)
    }
    return data
  }

  getBuffer() {
    let data = this.getRotate()
    let arr = [];

    for (let y = 0; y < this.height; y++) {
      for (let x = 0; x < this.width; x++) {
        arr = this.direction === 'serpentine' && y % 2 ? [...arr, ...data[x][y]] : [...arr, ...data[this.width - 1 - x][y]];
      }
    }

    let uint8bytes = Uint8Array.from(arr);
    let dataview = new DataView(uint8bytes.buffer);
    const buffer = [];

    for (let i = 0; i < dataview.byteLength; i += 4) {
      const value = dataview.getUint32(i);
      buffer.push(value);
    }
    return buffer;
  }

  update() {
    for (let y = 0; y < this.height; y++) {
      for (let x = 0; x < this.width; x++) {
        this.setColor(this.data[x][y]);
        this.draw(x, y);
      }
    }
  }

  setBuffer(buffer) {
    if (!buffer) return;

    for (let i = 0; i < buffer.length; i++) {
      const x = i % 16
      const y = (i / 16) | 0
      const value = int32ToBytes(buffer[i])
      if (this.direction === 'serpentine' && y % 2) {
        this.data[x][y] = value
      } else {
        this.data[this.width - 1 - x][y] = value
      }
    }

    this.data = this.getRotate()
    this.update()
  }

  erase(x, y) {
    var temp = this.color;
    var tga = this.ctx.globalAlpha;
    this.setColor(this.fill);
    this.draw(x, y);
    this.setColor(temp);
    this.ctx.globalAlpha = tga;
  }

  setColor(color) {
    this.ctx.globalAlpha = 1;
    this.color = color;
    this.ctx.fillStyle = `rgba(${this.color.join(',')})`;
  }

  setFill(color) {
    this.fill = color;
  }

  save() {
    this.canvas.toBlob(function (blob) {
      var url = URL.createObjectURL(blob);
      var link = document.createElement('a');
      link.download = 'canvas.png';
      link.href = url;
      link.click();
    });
    this.saveInLocal()
  }

  clear() {
    this.ctx.fillStyle = `rgba(${this.fill.join(',')})`;
    this.ctx.fillRect(0, 0, this.w, this.h);
    this.data = [...Array(this.height)].map(() => Array(this.width).fill(this.fill));
    this.setColor(this.color);
  }

  addFrame(data = null) {
    var img = new Image();
    img.src = data || this.canvas.toDataURL();
    this.frames.push([img, this.data.map(inner => inner.slice())]);
  }

  deleteFrame(f) {
    this.frames.splice(f, 1);
  }

  loadFrame(f) {
    this.clear();
    var img = this.frames[f][1];
    var tmp_color = this.color;
    var tmp_alpha = this.ctx.globalAlpha;
    this.ctx.globalAlpha = 1;
    var i, j;
    for (i = 0; i < this.width; i++) {
      for (j = 0; j < this.height; j++) {
        this.setColor(img[i][j]);
        this.draw(i, j);
      }
    }
    this.setColor(tmp_color);
    this.ctx.globalAlpha = tmp_alpha;
  }

  undo() {
    this.clear();
    this.redo_arr.push(this.steps.pop());
    this.steps.forEach(step => {
      this.setColor(step[2]);
      this.ctx.globalAlpha = step[3];
      this.draw(step[0], step[1], true);
    });
  }

  redo() {
    this.steps.push(this.redo_arr.pop());
    this.steps.forEach(step => {
      this.setColor(step[2]);
      this.ctx.globalAlpha = step[3];
      this.draw(step[0], step[1], true);
    });
  }

  saveInLocal() {
    const data = {
      colors: this.colors,
      currColor: this.color,
      width: this.width,
      height: this.height,
      url: this.canvas.toDataURL(),
      steps: this.steps,
      redo_arr: this.redo_arr,
    };
    localStorage.setItem('canvas-data', JSON.stringify(data));
  }

  loadInLocal() {
    const load = localStorage.getItem('canvas-data');
    if (load) {
      JSON.parse(load)
    }
    // const data = {
    //   colors: this.colors,
    //   currColor: this.color,
    //   width: this.width,
    //   height: this.height,
    //   url: this.canvas.toDataURL(),
    //   steps: this.steps,
    //   redo_arr: this.redo_arr,
    // };
  }

  addImage() {
    var _this = this;
    var fp = document.createElement('input');
    fp.type = 'file';
    fp.click();
    fp.onchange = function (e) {
      var reader = new FileReader();
      reader.readAsDataURL(e.target.files[0]);
      reader.onload = function () {
        var uimg = new Image();
        uimg.src = reader.result;
        uimg.width = _this.w;
        uimg.height = _this.h;
        uimg.onload = function () {
          var pxc = document.createElement('canvas');
          pxc.width = _this.w;
          pxc.height = _this.h;
          var pxctx = pxc.getContext('2d');
          pxctx.drawImage(uimg, 0, 0, _this.w, _this.h);
          var i, j;
          for (i = 0; i < _this.width; i++) {
            for (j = 0; j < _this.height; j++) {
              var ctr = 0;
              var avg = [0, 0, 0, 0];
              var pix = pxctx.getImageData(10 * i, 10 * j, 10, 10).data;
              pix.forEach((x, k) => {
                avg[k % 4] += x;
                if (k % 4 == 0) ctr++;
              });
              avg = avg.map(x => ~~(x / ctr));
              _this.setColor(avg);
              _this.draw(i, j);
            }
          }
        };
      };
    };
  }
}
