const root = '../data';
const RemovePlugin = require('remove-files-webpack-plugin');

module.exports = {
  outputDir: root,
  productionSourceMap: false,
  filenameHashing: false,
  devServer: {
    proxy: 'http://192.168.1.59/'
  },
  // configureWebpack: config => {
  //   if (process.env.NODE_ENV === 'production') {
  //   } else {
  //   }
  // },
  chainWebpack: config => { },
  configureWebpack: {
    optimization: {
      splitChunks: {
        cacheGroups: {
          default: false,
          // Merge all the CSS into one file
          styles: {
            name: 'styles',
            test: m => m.constructor.name === 'CssModule',
            chunks: 'all',
            minChunks: 1,
            enforce: true
          },
        },
      },
    },
    // css: {
    //   loaderOptions: {
    //     scss: {

    //     },
    //   },
    // },
    plugins: [
      new RemovePlugin({
        after: {
          root: root,
          trash: true,
          test: [
            {
              folder: './css',
              method: absoluteItemPath => {
                return new RegExp(/\.css$/, 'm').test(absoluteItemPath);
              },
            },
            {
              folder: './js',
              method: absoluteItemPath => {
                return new RegExp(/\.js$/, 'm').test(absoluteItemPath);
              },
              recursive: true,
            },
            {
              folder: './fonts',
              method: absoluteItemPath => {
                return new RegExp(/\.(ttf|woff|eot)$/, 'm').test(absoluteItemPath);
              },
              recursive: true,
            },
            {
              folder: './img',
              method: absoluteItemPath => {
                return new RegExp(/\.svg$/, 'm').test(absoluteItemPath);
              },
              recursive: true,
            },
          ],
        },
      }),
    ],
  },
  pluginOptions: {
    compression: {
      gzip: {
        filename: '[file].gz',
        algorithm: 'gzip',
        include: /\.(js|css|svg|ttf|woff|eot|json)$/i,
        minRatio: 1,
      },
    },
  },
};
