import Vue from 'vue'

Vue.filter('toByte', (value) => {
    const sizes = ['Bytes', 'KB', 'MB'];
    if (value == 0) return '0 Byte';
    const i = parseInt(Math.floor(Math.log(value) / Math.log(1024)));
    return Math.round(value / Math.pow(1024, i), 2) + ' ' + sizes[i];
})