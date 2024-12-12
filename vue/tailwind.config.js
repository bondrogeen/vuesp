/** @type {import('tailwindcss').Config} */
const colors = require('tailwindcss/colors')

module.exports = {
  content: [],
  purge: ['./index.html', './src/**/*.{vue,js,ts,jsx,tsx}'],
  darkMode: 'class',
  theme: {
    container: {
      screens: {
        md: '600px',
        lg: '960px',
        xl: '1260px',
        "2xl": '1440px',
      },
    },
    screens: {
      md: '600px',
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
    fontSize: {
      'xsmall': ['10px', { lineHeight: '140%', fontWeight: '400' }],
      'small': ['12px', { lineHeight: '140%', }],
      'body': ['14px', { lineHeight: '140%', }],
      'base': ['16px', { lineHeight: '140%', }],
      'lg': ['18px', { lineHeight: '140%', }],
      'xl': ['20px', { lineHeight: '140%', }],
      '2xl': ['24px', { lineHeight: '140%', }],
      '3xl': ['28px', { lineHeight: '140%', }],
      '4xl': ['32px', { lineHeight: '140%', }],

    },
    extend: {},
  },
  plugins: [],
}

