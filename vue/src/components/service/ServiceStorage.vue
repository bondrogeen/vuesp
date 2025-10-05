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

            <v-icons v-if="isLast(path, i)" name="Chevron" class="h-5 w-4 -rotate-90"></v-icons>
          </div>
        </div>

        <VFile multiple="multiple" @change="onUpload"></VFile>
      </div>

      <div class="relative min-h-[260px]">
        <v-loader v-if="isLoading" class="absolute top-1/2 left-1/2 text-primary" />

        <v-list v-slot="{ item: { name, size, isDir, isFile } }" className="" :list="sortFiles">
          <div class="flex items-center flex-auto" @click="onNext(isDir, name)">
            <div class="mr-4 text-gray-400">
              <v-icons name="Folder" v-if="isDir"></v-icons>
              <v-icons name="File" v-else></v-icons>
            </div>

            <div>
              <div class="text-sm">{{ isDir ? `${name}` : name }}</div>

              <div v-if="isFile" class="text-sm text-[10px] text-gray-400">{{ toByte(size) }} ({{ size }})</div>
            </div>
          </div>

          <v-dropdown right="0" left="unset" top="0">
            <template #activator="{ on }">
              <button @click="on.click">
                <v-icons name="Dots" class="rotate-90"></v-icons>
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
            <v-icons name="Dots" class="rotate-90"></v-icons>
          </v-button>
        </template>

        <v-list :list="mainMenu" @click="onEventService"></v-list>
      </v-dropdown>
    </Teleport>
  </div>
</template>

<script setup lang="ts">
import type { Ref } from 'vue';
import type { IListItem, IMessageFile, TypeMessage } from '@/types';
import { KEYS } from '@/types';

import { watchEffect, ref, computed, nextTick } from 'vue';

import { toByte, debounce, useFetch, createDownloadLink } from 'vuesp-components/helpers';
import { PATH_FS } from '@/utils/const';

import { useConnection } from '@/composables/useConnection';

import { VFile } from 'vuesp-components';

const files: Ref<IMessageFile[]> = ref([]);
const fullPath = computed(() => `${path.value.join('/').replace('root', '')}/`);

const mainMenu: IListItem[] = [
  { name: 'Upload', value: 2 },
  { name: 'Reload', value: 3 },
  { name: 'Format', value: 4 },
];
const listMenu: IListItem[] = [
  { name: 'Download', value: 1 },
  { name: 'Remove', value: 2 },
];

const isLoading = ref(false);

const path = ref(['root']);

const sortFiles = computed(() => JSON.parse(JSON.stringify(files.value)).sort((a: IMessageFile, b: IMessageFile) => ((a.isFile || 0) > (b.isFile || 0) ? 1 : -1)));

const fileName = (name: string) => `${fullPath.value}${name}`;
const getListMenu = (isDir: boolean) => listMenu.filter((i) => (isDir ? i.value !== 1 : true));
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

const onEventService = ({ value }: IListItem) => {
  if (value === 2) onClickUpload();
  if (value === 3) onUpdate();
  if (value === 4) onSureFormat();
};

const onEventList = (name: string, { value }: IListItem) => {
  if (value === 1) createDownloadLink(`${PATH_FS}?file=${fileName(name)}`, name);
  if (value === 2) onSureDelete(name);
};

const onFormat = async () => {
  const res = await useFetch.$post(`${PATH_FS}?format=true`);
  if (res?.state) onUpdate();
};

const onSureFormat = () => onDialog({ value: true, message: 'All files will be deleted. Are you sure?', callback: onFormat });

const onUpload = async (files: FileList | null) => {
  if (!files) return;
  let totalSize = 0;
  const body = new FormData();
  for (let i = 0; i < files.length; i++) {
    const file = files.item(i);
    if (!file) return;
    totalSize += file.size;
    body.append(`file[${i}]`, file, `${fullPath.value}${file.name}`);
  }
  const { totalBytes = 0, usedBytes = 0 } = main.value.info;
  if (totalSize < totalBytes - usedBytes) {
    const res = await useFetch.$post(PATH_FS, { body });
    if (res?.state) onUpdate();
  } else {
    onDialog({ value: true, message: 'No free space' });
  }
};

const onDelete = async (name: string) => {
  const res = await useFetch.$delete(`${PATH_FS}?file=${fileName(name)}`);
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

const onLoad = debounce(() => {
  isLoading.value = false;
}, 300);

watchEffect(() => {
  onLoad(files.value);
});
</script>
