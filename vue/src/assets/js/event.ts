interface TypeEvent {
  [key: string]: any;
}

class Event {
  events: TypeEvent;
  constructor() {
    this.events = {};
  }

  on(name: string, fn: () => void) {
    this.events[name] = this.events[name] || [];
    this.events[name].push(fn);
  }

  off(name: string, fn: () => void) {
    if (this.events[name]) {
      for (var i = 0; i < this.events[name].length; i++) {
        if (this.events[name][i] === fn) {
          this.events[name].splice(i, 1);
          break;
        }
      }
    }
  }

  emit(name: string, data?: any) {
    if (this.events[name]) {
      this.events[name].forEach(function (fn: any) {
        fn(data);
      });
    }
  }
}

export default new Event();
