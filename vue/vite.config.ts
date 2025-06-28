import { defineConfig, loadEnv } from 'vite';
import vue from '@vitejs/plugin-vue';
import tailwindcss from '@tailwindcss/vite';
import { fileURLToPath, URL } from 'node:url';
import VueRouter from 'unplugin-vue-router/vite';
import { compression } from 'vite-plugin-compression2';

const path = fileURLToPath(new URL('./src', import.meta.url));

// https://vite.dev/config/
export default defineConfig(({ mode }) => {
  const env = loadEnv(mode, process.cwd(), 'VITE_');
  console.log(env);
  return {
    build: {
      outDir: env.VITE_OUTPUT_DIR,
      filenameHashing: false,
      cssCodeSplit: false,
      rollupOptions: {
        output: {
          inlineDynamicImports: true,
          entryFileNames: `assets/[name].js`,
          chunkFileNames: `assets/[name].js`,
          assetFileNames: `assets/[name].[ext]`,
        },
      },
    },
    plugins: [
      VueRouter(),
      vue(),
      tailwindcss(),
      compression({
        filename: '[path][base].gz',
        algorithm: 'gzip',
        deleteOriginalAssets: true,
        include: /\.(js|css|svg|ttf|woff|eot|json)$/i,
        threshold: 0,
      }),
    ],
    resolve: {
      extensions: ['.mjs', '.js', '.ts', '.jsx', '.tsx', '.json', '.vue'],
      alias: {
        '@': path,
      },
    },
    server: {
      allowedHosts: ['vuesp.ru', 'test.vuesp.ru', 'home.vuesp.ru'],
      host: '0.0.0.0',
      port: 3000,
      proxy: {
        '/esp': {
          target: `ws://${env.VITE_PROXY}`,
          ws: true,
        },
        '/get': {
          target: `http://${env.VITE_PROXY}`,
        },
        '/fs': {
          target: `http://${env.VITE_PROXY}`,
        },
        '/upload': {
          target: `http://${env.VITE_PROXY}`,
        },
        '/update': {
          target: `http://${env.VITE_PROXY}`,
        },
      },
    },
  };
});
