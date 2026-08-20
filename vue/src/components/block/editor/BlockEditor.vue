<script setup lang="ts">
import type { IBlockEditorProps, IBlockEditorEmit } from './types';
import type { Suggestion } from './plugins/types';
import { onMounted, onUnmounted, watch } from 'vue';
import { Yace } from 'yace';
import { history, preserveIndent } from 'yace/plugins';
import { autoSemicolon, autocomplete } from './plugins';
import { fullHighlighter } from './highlighters';

import { formatScript, removeLBScript } from './format';

const { value = '' } = defineProps<IBlockEditorProps>();
const emit = defineEmits<IBlockEditorEmit>();

let editor: Yace;

const customSuggestions: Suggestion[] = [
  { label: 'myFunction', type: 'function', insertText: 'myFunction()', detail: 'моя функция' },
  { label: 'MY_CONST', type: 'variable', detail: 'константа' },
  { label: 'btn_5', type: 'event', detail: 'кнопка 5' },
];

watch(
  () => value,
  (v) => {
    editor.update({ value: formatScript(v) });
  }
);

onMounted(() => {
  editor = new Yace('#editor', {
    value,
    lineNumbers: true,
    highlighters: [fullHighlighter],
    plugins: [
      history(),
      preserveIndent(),
      autocomplete({
        customSuggestions,
        minPrefixLength: 1,
        maxSuggestions: 20,
        i18n: (item) => `${item?.detail} 9090` || '',
      }),
      autoSemicolon(),
    ],
    styles: { fontSize: '14px' },
  });

  editor.textarea.addEventListener('blur', (e: FocusEvent) => {
    const target = e.currentTarget as HTMLTextAreaElement;
    const value = target.value || '';
    emit('update', removeLBScript(value));
  });
});

onUnmounted(() => {
  editor.destroy();
});
</script>

<template>
  <div class="h-50 overflow-auto scrollbar">
    <div class="min-h-50" id="editor"></div>
  </div>
</template>

<style>
.tok--comment {
  color: #6a9955;
  font-style: italic;
} /* Зеленый, курсив */
.tok--string {
  color: #ce9178;
} /* Оранжевый */
.tok--number {
  color: #52ce0f;
} /* Светло-зеленый */
.tok--keyword {
  color: #a646a0;
  font-weight: bold;
} /* Синий, жирный */
.tok--variable {
  color: #4993ff;
} /* Голубой */
.tok--event {
  color: #dcdcaa;
} /* Фиолетовый */

/* Дополнительно: операторы можно подсветить отдельно */
.tok--operator {
  color: #eed600;
}

/* Строка с номером строки (если используете) */
.yace-line-number {
  color: #858585;
  user-select: none;
}

.yace-autocomplete {
  background: #252526;
  border: 1px solid #454545;
  border-radius: 6px;
  max-height: 250px;
  overflow-y: auto;
  min-width: 250px;
  max-width: 400px;
  z-index: 10000;
  font-family: 'Consolas', 'Monaco', 'Courier New', monospace;
  font-size: 14px;
  box-shadow: 0 8px 24px rgba(0, 0, 0, 0.5);
  padding: 4px 0;
}

.yace-autocomplete::-webkit-scrollbar {
  width: 8px;
}

.yace-autocomplete::-webkit-scrollbar-track {
  background: #252526;
}

.yace-autocomplete::-webkit-scrollbar-thumb {
  background: #454545;
  border-radius: 4px;
}

.yace-autocomplete::-webkit-scrollbar-thumb:hover {
  background: #555555;
}

.yace-autocomplete-item {
  padding: 6px 12px;
  cursor: pointer;
  display: flex;
  justify-content: space-between;
  align-items: center;
  transition: background 0.1s;
  border-left: 2px solid transparent;
}

.yace-autocomplete-item:hover {
  background: #2a2d2e;
}

.yace-autocomplete-item.selected {
  background: #094771;
  border-left-color: #0078d4;
}
</style>
