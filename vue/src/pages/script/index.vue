<template>
  <div class="container mx-auto">
    <div class="flex flex-col gap-3 sm:gap-4 mx-auto">
      <div class="flex justify-between">
        <h1>{{ $t('menu.script') }}</h1>

        <v-dropdown right="0" left="unset" top="0">
          <template #activator="{ on }">
            <v-button color="" type="icon" @click="on.click">
              <icon-ri-more-line class="rotate-90"></icon-ri-more-line>
            </v-button>
          </template>

          <v-list :items="listMenu" @click="onMenu" />
        </v-dropdown>
      </div>

      <div class="flex flex-col sm:flex-row gap-3 sm:gap-4">
        <aside class="w-full sm:w-56 md:w-64 lg:w-72 flex-shrink-0 flex flex-col">
          <card-main :title="$t('list')">
            <template #header>
              <v-button color="transparent" class="size-6" size="icon" @click="onDialog">
                <icon-ri-sticky-note-add-line class="text-gray-500"></icon-ri-sticky-note-add-line>
              </v-button>
            </template>

            <ul class="overflow-y-auto flex flex-col rounded-md border border-gray-200 dark:border-gray-800 bg-gray-50 dark:bg-gray-900 p-2">
              <li v-for="script of scripts" :key="script.id" class="group text-sm cursor-pointer transition-all flex items-center justify-between py-2" @click="onSelect(script)">
                <div class="flex items-center gap-2 min-w-0">
                  <span>#{{ script.id }}</span>
                  <span class="truncate">{{ script.name }}</span>
                </div>

                <div class="group flex items-center gap-1">
                  <v-button color="transparent" class="size-6" @click.stop="onRemove(script)">
                    <icon-ri-delete-bin-line class="text-gray-500 hover:text-red-400 group-hover:opacity-100 opacity-40 transition-all"></icon-ri-delete-bin-line>
                  </v-button>
                </div>
              </li>
              <li v-if="scripts.length === 0" class="text-sm text-slate-400 italic px-3 py-4 text-center">{{ $t('noScripts') }}</li>
            </ul>

            <div class="px-2 border-t border-gray-200 dark:border-gray-700 text-slate-400 flex justify-between mt-4 text-xs">
              <span>{{ scripts.length }} скриптов</span>
            </div>

            <ul>
              <!-- <li v-for="item of examples"></li> -->
            </ul>
          </card-main>
        </aside>

        <div class="flex-1 flex flex-col gap-3 sm:gap-4 min-h-0">
          <card-main :title="`${$t('editor')}  ${selectedScript?.name ? selectedScript?.name : ''}`">
            <template #header>
              <div class="flex gap-3">
                <v-button color="transparent" class="size-6" size="icon" :disabled="selectedScript?.content === content" @click="onSaveScript()">
                  <icon-ri-save-line class="text-gray-500"></icon-ri-save-line>
                </v-button>
                <v-button color="transparent" class="size-6" size="icon" :disabled="!content" @click="onRunScript()">
                  <icon-ri-play-line class="text-gray-500"></icon-ri-play-line>
                </v-button>
              </div>
            </template>

            <div class="flex-auto bg-gray-50 dark:bg-gray-900 dark:border-gray-700 left-0 w-full sticky top-0">
              <textarea
                v-model="content"
                class="w-full h-full min-h-50 p-4 text-sm outline-0 relative rounded-md border border-gray-200 dark:border-gray-800 flex flex-col field-sizing-content"
                placeholder="Выберите скрипт"
                spellcheck="false"
              ></textarea>
            </div>

            <div class="text-sm border-t border-gray-200 dark:border-gray-700 text-slate-400 flex flex-wrap justify-between mt-4">
              <span>
                <i class="far fa-file-alt mr-1"></i>
                {{ selectedScript ? `длина: ${content?.length || 0} символов` : 'нет скрипта' }}
              </span>

              <!-- <ul>
                <li v-for="error of errors" :key="error.position">{{ $t(`error.${error.key}`) }}</li>
              </ul> -->
            </div>
          </card-main>

          <card-main :title="$t('logs')">
            <template #header>
              <v-button color="transparent" class="size-6 opacity-60 hover:opacity-100" size="icon" @click="logs = []">
                <icon-ri-eraser-line class="text-gray-500"></icon-ri-eraser-line>
              </v-button>
            </template>

            <ul ref="container" class="overflow-y-auto flex flex-col rounded-md border border-gray-200 dark:border-gray-800 bg-gray-50 dark:bg-gray-900 p-4 max-h-50 min-h-32 scrollbar">
              <li v-for="(log, idx) in logs" :key="idx" class="py-0.5 flex items-start gap-2 border-b border-slate-100/50 last:border-0 text-sm">
                <span class="text-slate-400 whitespace-nowrap">{{ log.time }}</span>
                <span :class="'text-emerald-600'">
                  {{ log.text }}
                </span>
              </li>
              <li v-if="logs.length === 0" class="text-sm text-slate-400 italic px-3 py-4 text-center">{{ $t('noLogs') }}</li>
            </ul>
          </card-main>
        </div>
      </div>
    </div>

    <v-dialog v-if="dialogAdd" size="sm" :title="$t('add')" @close="dialogAdd = false">
      <div class="my-4">
        <v-text-field v-model="id" v-bind="idAttrs" :label="$t('id')" disabled />

        <v-text-field v-model="name" v-bind="nameAttrs" :label="$t('name')" />

        <v-button class="w-full" color="blue" :disabled="!id || !name" @click="onSubmit">
          {{ $t('add') }}
        </v-button>
      </div>
    </v-dialog>
  </div>
</template>

<script setup lang="ts">
import type { IScript, ILog } from './types';
import { computed, nextTick, onMounted, ref, useTemplateRef, watch } from 'vue';
import { timeUtcToString } from 'vuesp-components/helpers';
// import { ScriptValidatorAPI } from '../../assets/js/ScriptValidator';

// import { KEYS } from '@/types';
// import { COMMAND } from '@/utils/gpio';

import { useConnection } from '@/composables/useConnection';
import { required, maxLen, minVal, maxVal } from '@/utils/validate';

import { useForm } from 'vuesp-components/composables';
import type { IListItem, ValidationSchema } from 'vuesp-components/types';

import { useLocale } from '@/composables/useLocale';
import { useFetch } from '@vueuse/core';

const { $t } = useLocale();

const { message, onSend } = useConnection(() => {
  // send(KEYS.PORT, { gpio: 0, command: COMMAND.GPIO_COMMAND_GET_ALL });
});

const container = useTemplateRef('container');

const dialogAdd = ref(false);
const logs = ref<ILog[]>([]);

const listMenu: IListItem[] = [
  { name: $t('save'), value: 1 },
  { name: $t('restore'), value: 2 },
];

const onMenu = (item: IListItem) => {
  if (item.value === 1) onSave();
  if (item.value === 2) onLoad();
};

const { defineField, handleSubmit } = useForm({
  validationSchema: () =>
    ({
      id: [required, minVal(1), maxVal(255)],
      name: [required, maxLen(8)],
    }) as ValidationSchema,
});

const [id, idAttrs] = defineField<string>('id');
const [name, nameAttrs] = defineField<string>('name');

const addScript = () => {
  scripts.value.push({ id: +id.value, name: name.value, content: '' });
};

const onSubmit = handleSubmit(() => {
  addScript();
  name.value = ``;
  dialogAdd.value = false;
});

const ids = computed(() => scripts.value.map(({ id }) => id));

const onDialog = () => {
  const getId = Array.from({ length: 256 }, (_, i) => i + 1).find((i) => !ids.value.includes(i));
  id.value = `${getId}`;
  dialogAdd.value = true;
};
const onRemove = ({ id }: IScript) => {
  scripts.value = scripts.value.filter((i) => i.id !== id);
};
const onSaveScript = () => {
  if (!selectedScript.value) return;
  const { id } = selectedScript.value;
  scripts.value = scripts.value.map((i) => (i.id === id ? { ...i, content: content.value } : i));
};

const scrollLastLog = () => {
  if (!container.value?.lastElementChild) return;
  container.value.lastElementChild.scrollIntoView({
    behavior: 'smooth',
    block: 'end',
  });
};

watch(
  () => message.value,
  (v) => {
    if (v.type === 0) {
      const date = new Date();
      logs.value.push({ time: `${timeUtcToString(date, { minute: '2-digit', hour: '2-digit', second: '2-digit' })}`, type: 0, text: v.text });

      nextTick(() => {
        scrollLastLog();
      });
    }
  }
);

const onRunScript = () => {
  const id = selectedScript.value?.id || 0;
  onSend('MESSAGE', { ...message.value, id, type: 2, text: content.value });
};

const scripts = ref<IScript[]>([]);
const selectedScript = ref<IScript | null>(null);
const content = ref('');

function onSelect(script: IScript) {
  selectedScript.value = script;
  content.value = script.content;
}

// const validate = computed(() => ScriptValidatorAPI.validate(selectedScriptContent.value || ''));
// const isValid = computed(() => validate.value.valid);
// const errors = computed(() => validate.value.errors);

const PATH = '/scripts.txt';

const onLoad = async () => {
  const { data } = await useFetch(`/fs?file=${PATH}`).text();

  if (!data.value) return;
  const lines = data.value.split('\n');
  scripts.value = lines.map((script): IScript => {
    const [id, name, content] = script.split(':', 3);
    return { id: +id, name, content };
  });
};

const onSave = async () => {
  const text = scripts.value.map(({ id, name, content }) => `${id}:${name}:${content.replace(/(\r\n|\n|\r)/g, '')}`).join('\n');
  const body = new FormData();
  body.append('file[0]', new Blob([text], { type: 'text/plain' }), PATH);
  return await useFetch('/fs', { body }).post();
};

onMounted(() => {
  onLoad();
});
</script>

<!-- 
if:$v0=5;$v0=0;end;
if:$v0<10;$v0=$v0+1;end;
if:$v0>=0;$display='POS';end;
if:$v0<=0;$display='NEG';end;
if:$v0!=0;$display='NOT ZERO';end;if:$s0!='OFF';$p13=255;end;
if:$v0>5&&$v0<10;$display='BETWEEN';end;
if:$v0==0||$v1==0;$display='ZERO';end;


$v0=1;$v1=1;if:$v0==1;if:$v1==1;$display='BOTH';end;end;
$v0=5;if:$v0>0;if:$v0<10;$display='BETWEEN';end;end;
$v0=$p13;if:$v0==0;$display='ZERO';else;if:$v0<128;$display='LOW';else;$display='HIGH';end;end;
$v0=0;while:$v0<5;$v0=$v0+1;end;
$v0=10;while:$v0>0;$v0=$v0-1;end;
$v0=0;while:$v0<10;$v0=$v0+1;if:$v0==5;$v0=10;end;end;
$v0=0;while:$v0<10;$p13=$v0;$v0=$v0+1;end;
$v0=0;while:$v0<256;$p13=$v0;$v0=$v0+10;end;
$v0=0;while:$v0<100;$v0=$v0+1;if:$v0>50;$v0=100;end;end;
$v0=get($a0,0);
$v0=get($a0,1);
$v0=len($a0);
set($a0,$v0);
$s0=chr(65);
$v0=ord($s0);
$v0=len($s0);
$s0=$s0+'World';
$s0=$s0+' ';
if:$v0>128;$p13=0;else;$p13=255;end;
while:$v0<10;$p13=$v0;$v0=$v0+1;end;
while:$v0<256;$p13=$v0;$v0=$v0+1;end;
$p13=255;wait(1000);$p13=0;wait(500);
$p13=255;wait(5s);$p13=0;wait(1m);
wait(100u);
wait(100);
wait(1s);
wait(5s);
wait(1m);
wait(1h);
on('EVENT',if:$p13==0;call(1);else;call(2);end;end);
$display=$s0+'World';
$v0=0;while:$v0<5;$display=$v0;$v0=$v0+1;end;
on('CLICK',$display='CLICKED';end);
on('START',$p13=255;wait(1000);$p13=0;end);
on('BUTTON',if:$p13==0;$display='LOW';end;end);
on('STOP',$p13=0;$display='STOPPED';end);
on('ALARM',$v0=0;while:$v0<5;$p13=255;wait(100);$p13=0;wait(100);$v0=$v0+1;end;end);
on('RESET',$v0=0;$p13=0;$display='RESET';end);
on('BUTTON',$p13=255;wait(1000);$p13=0;end);
if:$p0==0;$v0=$v0+1;$display=$v0;wait(200);end;
if:$temperature>25;$p13=255;else;$p13=0;end;
if:$s0=='START';while:$v0<10;$p13=$v0;$v0=$v0+1;end;end;
$v0=0;while:$v0<10;if:$v0%2==0;$display=$v0;end;$v0=$v0+1;end;
$v0=10;while:$v0>0;$display=$v0;$v0=$v0-1;wait(1000);end;$display='DONE';
$v0=0;while:$v0<5;$p13=255;wait(100s);$p13=0;wait(100s);$v0=$v0+1;end;
 -->
