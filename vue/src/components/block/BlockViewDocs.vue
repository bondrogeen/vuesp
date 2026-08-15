<template>
  <div class="max-w-4xl mx-auto">
    <div class="text-sm" v-if="isLoading || error">
      <span v-if="isLoading" class="text-gray-500">⏳ Загрузка...</span>
      <span v-if="error" class="text-red-600">❌ {{ error }}</span>
    </div>

    <div class="docs-content" v-html="renderedHtml"></div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue';

import { useLocale } from '@/composables/useLocale';
const { getLocale } = useLocale();

const BASE_URL = 'https://raw.githubusercontent.com/bondrogeen/vuesp/refs/heads/master/github/docs/scriptrunner/';
const MARKED_CDN = 'https://cdn.jsdelivr.net/npm/marked/lib/marked.umd.js';

interface MetaData {
  version?: string;
  updated?: string;
}

interface MarkedLib {
  parse: (markdown: string) => string;
  setOptions: (options: Record<string, any>) => void;
}

declare global {
  interface Window {
    marked?: MarkedLib;
  }
}

const rawMarkdown = ref<string>('');
const renderedHtml = ref<string>('');
const meta = ref<MetaData | null>(null);
const isLoading = ref<boolean>(false);
const error = ref<string | null>(null);
let markedLib: MarkedLib | null = null;

const loadMarked = (): Promise<MarkedLib> => {
  return new Promise((resolve, reject) => {
    if (window.marked) {
      window.marked.setOptions({
        gfm: true,
        breaks: true,
        headerIds: false,
      });
      resolve(window.marked);
      return;
    }

    const script = document.createElement('script');
    script.src = MARKED_CDN;
    script.async = true;

    script.onload = () => {
      if (window.marked) {
        window.marked.setOptions({
          gfm: true,
          breaks: true,
          headerIds: false,
        });
        resolve(window.marked);
      } else {
        reject(new Error('Marked library not found after loading'));
      }
    };

    script.onerror = () => {
      reject(new Error('Failed to load Marked library from CDN'));
    };

    document.head.appendChild(script);
  });
};

const parseMeta = (text: string): MetaData => {
  const lines = text.split('\n');
  const metaData: MetaData = {};
  let inMeta = true;

  for (const line of lines) {
    if (inMeta && line.startsWith('#')) {
      inMeta = false;
      continue;
    }
    if (inMeta && line.includes(':')) {
      const [key, value] = line.split(':').map((s) => s.trim());
      if (key && value) {
        if (key.toLowerCase().includes('version')) metaData.version = value;
        if (key.toLowerCase().includes('обновлено') || key.toLowerCase().includes('updated')) {
          metaData.updated = value;
        }
      }
    }
  }
  return metaData;
};

const loadDocs = async (langCode: string): Promise<void> => {
  if (!markedLib) {
    error.value = 'Библиотека Marked не загружена';
    return;
  }

  isLoading.value = true;
  error.value = null;
  renderedHtml.value = '';

  try {
    const url = `${BASE_URL}${langCode}.md`;
    const response = await fetch(url);

    if (!response.ok) {
      throw new Error(`HTTP ${response.status}: ${response.statusText}`);
    }

    const text = await response.text();
    rawMarkdown.value = text;
    meta.value = parseMeta(text);
    renderedHtml.value = markedLib.parse(text);
  } catch (err: unknown) {
    const errorMessage = err instanceof Error ? err.message : 'Неизвестная ошибка';
    error.value = errorMessage;
    console.error('Error loading docs:', err);
  } finally {
    isLoading.value = false;
  }
};

onMounted(async () => {
  try {
    markedLib = await loadMarked();
    const detectedLang = getLocale().startsWith('ru') ? 'ru' : 'en';
    await loadDocs(detectedLang);
  } catch (err: unknown) {
    const errorMessage = err instanceof Error ? err.message : 'Неизвестная ошибка';
    error.value = errorMessage;
    console.error('Initialization error:', err);
  }
});
</script>