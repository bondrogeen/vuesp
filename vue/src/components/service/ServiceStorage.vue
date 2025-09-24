<template>
  <div class="grid grid-cols-1 xl:grid-cols-2">
    <card-gray title="File system">
      <div class="px-4 pb-2 flex items-center border-b border-gray-200 dark:border-gray-600">
        <div class="flex gap-2 items-center font-bold flex-auto">
          <div
            v-for="(value, i) of path"
            :key="value"
            class="flex items-center gap-2 cursor-pointer text-gray-600 last:cursor-default last:text-gray-900 dark:text-gray-200 dark:last:text-white"
            @click="onPrev(i)"
          >
            <div>{{ value }}</div>

            <v-icons name="IconChevron" v-if="isLast(path, i)" class="h-5 w-4 -rotate-90"></v-icons>
          </div>
        </div>

        <div>
          <VTextFieldFile @change="onUpload"></VTextFieldFile>
        </div>
      </div>

      <div class="relative min-h-[260px]">
        <v-loader v-if="isLoading" class="absolute top-1/2 left-1/2 text-primary" />

        <v-list v-slot="{ item: { name, size, isDir, isFile } }" :list="sortFiles">
          <div class="flex items-center flex-auto" @click="onNext(isDir, name)">
            <div class="mr-4 text-gray-400">
              <v-icons name="IconFolder" v-if="isDir"></v-icons>
              <v-icons name="IconFile" v-else></v-icons>
            </div>

            <div>
              <div class="text-sm">{{ isDir ? `${name}` : name }}</div>

              <div v-if="isFile" class="text-sm text-[10px] text-gray-400">{{ toByte(size) }} ({{ size }})</div>
            </div>
          </div>

          <v-dropdown right="0" left="unset" top="0">
            <template #activator="{ on }">
              <button @click="on.click">
                <v-icons name="IconDots" class="rotate-90"></v-icons>
              </button>
            </template>

            <v-list :list="getListMenu(isDir)" @click="onEventList(name, $event)" />
          </v-dropdown>
        </v-list>
      </div>
    </card-gray>

    <Teleport to="[data-slot='device']">
      <v-dropdown right="0" left="unset" top="0">
        <template #activator="{ on }">
          <v-button color="" type="icon" @click="on.click">
            <v-icons name="IconDots" class="rotate-90"></v-icons>
          </v-button>
        </template>

        <v-list :list="mainMenu" @click="onEventService"></v-list>
      </v-dropdown>
    </Teleport>
  </div>
</template>

<script setup lang="ts">
import type { Ref } from 'vue';
import type { IListItem, ITextFieldEvent, IMessageFile, TypeMessage } from '@/types';
import { KEYS } from '@/types';

import { watchEffect, ref, computed, nextTick } from 'vue';

import { toByte, debounce, useFetch, createDownloadLink } from 'vuesp-components/helpers';

import { useConnection } from '@/composables/useConnection.js';

import { VTextFieldFile } from 'vuesp-components';

const files: Ref<IMessageFile[]> = ref([]);
const fullPath = computed(() => `${path.value.join('/').replace('root', '')}/`);

const URL = '/fs';

const mainMenu: IListItem[] = [
  { id: 2, name: 'Upload' },
  { id: 3, name: 'Reload' },
  { id: 4, name: 'Format' },
];
const listMenu: IListItem[] = [
  { id: 1, name: 'Download' },
  { id: 2, name: 'Remove' },
];

const isLoading = ref(false);

const path = ref(['root']);

const sortFiles = computed(() => JSON.parse(JSON.stringify(files.value)).sort((a: IMessageFile, b: IMessageFile) => ((a.isFile || 0) > (b.isFile || 0) ? 1 : -1)));

const fileName = (name: string) => `${fullPath.value}${name}`;
const getListMenu = (isDir: boolean) => listMenu.filter((i) => (isDir ? i.id !== 1 : true));
const isLast = (path: string[], i: number) => path.length > i + 1;

const onMessage = ({ key, object }: TypeMessage) => {
  if (key === 'FILES' && object) files.value.push(object);
};

const { main, onSend, onDialog } = useConnection((send) => {
  send(KEYS.FILES, { command: 0, name: fullPath.value });
}, onMessage);

const onUpdate = () => {
  isLoading.value = true;
  files.value = [];
  onSend(KEYS.FILES, { command: 0, name: fullPath.value });
  onSend(KEYS.INFO);
};

const onPrev = (index: number) => {
  if (path.value.length > index + 1) {
    path.value = path.value.filter((_, i) => i < index + 1);
    nextTick(() => onUpdate());
  }
};

const onNext = (isDir: boolean, value: string) => {
  if (isDir && path) {
    path.value.push(value);
    onUpdate();
  }
};

const onClickUpload = () => {
  const el: any = document.querySelector('input[type="file"]');
  if (el) el.click();
};

const onEventService = ({ id }: IListItem) => {
  if (id === 2) onClickUpload();
  if (id === 3) onUpdate();
  if (id === 4) onSureFormat();
};

const onEventList = (name: string, { id }: IListItem) => {
  if (id === 1) createDownloadLink(`${URL}?file=${fileName(name)}`, name);
  if (id === 2) onSureDelete(name);
};

const onFormat = async () => {
  const res = await useFetch.post(`${URL}?format=true`).then((r) => r.json());
  if (res?.state) onUpdate();
};

const onSureFormat = () => onDialog({ value: true, message: 'All files will be deleted. Are you sure?', callback: onFormat });

const onUpload = async (data: ITextFieldEvent) => {
  const totalSize = data?.info?.totalSize || 0;
  const files = data?.files || [];

  const body = new FormData();
  for (let i = 0; i < files.length; i++) {
    const file: File | null = files.item(i);
    if (file) {
      const fileName = `${fullPath.value}${file.name}`;
      body.append(`file[${i}]`, file, fileName);
    }
  }
  const { totalBytes = 0, usedBytes = 0 } = main.value.info;

  if (totalSize < totalBytes - usedBytes) {
    const res = await useFetch.post(URL, { body }).then((r) => r.json());
    if (res?.state) onUpdate();
  } else {
    onDialog({ value: true, message: 'No free space' });
  }
};

const onDelete = async (name: string) => {
  const res = await useFetch.delete(`${URL}?file=${fileName(name)}`).then((r) => r.json());
  if (res?.state) onUpdate();
  else onDialog({ value: true, message: 'Directory is not empty' });
};

const onSureDelete = (name: string) => {
  if (fileName(name).includes('www')) {
    onDialog({ value: true, message: 'The file belongs to the "www" directory. <br/> Are you sure you want to delete it?', callback: onDelete.bind(this, name) });
  } else {
    onDelete(name);
  }
};

// const getFile = async (path: string) => await useFetch.get(`/fs?file=${path}`).then((r) => r.text());

const onLoad = debounce(() => {
  isLoading.value = false;
}, 300);

watchEffect(() => {
  onLoad(files.value);
});
</script>
