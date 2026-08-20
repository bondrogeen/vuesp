// suggestions.ts
import type { Suggestion } from './types';

export const DEFAULT_SUGGESTIONS: Suggestion[] = [
  // Ключевые слова
  { label: 'on', type: 'keyword', insertText: 'on(${событие})', detail: 'событие' },
  { label: 'if', type: 'keyword', insertText: 'if:${условие};', detail: 'условие' },
  { label: 'else', type: 'keyword', detail: 'иначе' },
  { label: 'end', type: 'keyword', detail: 'конец блока' },
  { label: 'while', type: 'keyword', insertText: 'while:${условие};', detail: 'цикл' },

  // Функции
  { label: 'wait', type: 'function', insertText: 'wait(${1s})', detail: 'задержка' },
  { label: 'log', type: 'function', insertText: 'log(${сообщение})', detail: 'логирование' },
  { label: 'len', type: 'function', insertText: 'len(${массив})', detail: 'длина' },
  { label: 'set', type: 'function', insertText: 'set(${массив}, ${индекс}, ${значение})', detail: 'массив' },
  { label: 'get', type: 'function', insertText: 'get(${массив}, ${индекс})', detail: 'массив' },
  { label: 'ord', type: 'function', insertText: 'ord(${строка})', detail: 'код символа' },
  { label: 'chr', type: 'function', insertText: 'chr(${код})', detail: 'символ по коду' },
  { label: 'read_temp', type: 'function', insertText: 'read_temp(${пин})', detail: 'температура' },
  { label: 'read_hum', type: 'function', insertText: 'read_hum(${пин})', detail: 'влажность' },
  { label: 'read_motion', type: 'function', insertText: 'read_motion(${пин})', detail: 'движение' },

  // События
  { label: 'btn_13', type: 'event', detail: 'кнопка' },
  { label: 'btn_13_1', type: 'event', detail: 'нажатие' },
  { label: 'btn_13_0', type: 'event', detail: 'отпускание' },
];

for (let i = 0; i <= 9; i++) {
  DEFAULT_SUGGESTIONS.push(
    { label: `$v${i}`, type: 'variable', detail: 'целое' },
    { label: `$i${i}`, type: 'variable', detail: 'знаковое' },
    { label: `$f${i}`, type: 'variable', detail: 'float' },
    { label: `$s${i}`, type: 'variable', detail: 'строка' },
    { label: `$p${i}`, type: 'variable', detail: 'порт' }
  );
}
