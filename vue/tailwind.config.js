/** @type {import('tailwindcss').Config} */
const colors = require('tailwindcss/colors')

module.exports = {
  content: [],
  purge: ['./index.html', './src/**/*.{vue,js,ts,jsx,tsx}'],
  theme: {
    screens: {
      sm: '360px',
      md: '768px',
      lg: '960px',
      xl: '1260px',
      "2xl": '1440px',
    },
    colors: {
      ...colors,
      'primary': '#24c0eb',
      'secondary': '#66d3f1',
      'black': '#000',
      'white': '#fff',
      'overlay': '#0c0b0ba1'
    },
    extend: {},
  },
  plugins: [],
}

