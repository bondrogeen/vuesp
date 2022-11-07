class Event {
  constructor() {
    this.events = {};
  }

  on(name, fn) {
    this.events[name] = this.events[name] || [];
    this.events[name].push(fn);
  }

  off(name, fn) {
    if (this.events[name]) {
      for (var i = 0; i < this.events[name].length; i++) {
        if (this.events[name][i] === fn) {
          this.events[name].splice(i, 1);
          break;
        }
      }
    }
  }

  emit(name, data) {
    if (this.events[name]) {
      this.events[name].forEach(function (fn) {
        fn(data);
      });
    }
  }
}

export default new Event();
