const toByte = value => {
  const sizes = ['B', 'KB', 'MB'];
  if (value == 0) return '0 B';
  let i = parseInt(Math.floor(Math.log(value) / Math.log(1024)));
  if (i > 1) i = 1;
  return Math.round(value / Math.pow(1024, i), 2) + ' ' + sizes[i];
};

const debounce = function (func, wait, immediate) {
  let timeout;
  return function executedFunction() {
    const context = this;
    const args = arguments;
    const later = function () {
      timeout = null;
      if (!immediate) func.apply(context, args);
    };
    const callNow = immediate && !timeout;
    clearTimeout(timeout);
    timeout = setTimeout(later, wait);
    if (callNow) func.apply(context, args);
  };
};

export { toByte, debounce };
