const { defineConfig } = require('@vue/cli-service');
const RemovePlugin = require('remove-files-webpack-plugin');
const VueAutoRoutingPlugin = require('vue-auto-routing/lib/webpack-plugin');
const Dotenv = require('dotenv-webpack');
const root = process.env.OUTPUT_DIR;

module.exports = defineConfig({
  transpileDependencies: true,
  outputDir: root,
  productionSourceMap: false,
  filenameHashing: false,
  css: {
    loaderOptions: {
      sass: {
        additionalData: `@import "@/assets/scss/variables.scss";`,
      },
    },
  },
  devServer: {
    host: '0.0.0.0',
    port: 3000,
    proxy: {
      '/esp': {
        target: `ws://${process.env.PROXY}`,
        ws: true,
      },
      '/fs': {
        target: `http://${process.env.PROXY}`,
      },
      '/upload': {
        target: `http://${process.env.PROXY}`,
      },
      '/update': {
        target: `http://${process.env.PROXY}`,
      },
    },
  },
  chainWebpack: () => {},
  configureWebpack: {
    performance: {
      hints: false,
      maxEntrypointSize: 512000,
      maxAssetSize: 512000,
    },
    optimization: {
      splitChunks: {
        cacheGroups: {
          default: false,
          styles: {
            name: 'styles',
            test: m => m.constructor.name === 'CssModule',
            chunks: 'all',
            minChunks: 1,
            enforce: true,
          },
        },
      },
    },
    plugins: [
      new Dotenv(),
      new VueAutoRoutingPlugin({
        pages: 'src/pages',
        importPrefix: '@/pages/',
      }),
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
});
