<template>
  <div class="grid grid-cols-1 xl:grid-cols-2 gap-4">
    <VCardGray title="Update">
      <div class="mb-4 flex items-center">
        <div class="md:flex flex-auto gap-4">
          <h6 class="text-gray-600 bg:text-gray-400">Firmware:</h6>

          <v-text-field-file v-slot="{ files }" accept=".bin" @change="onUpdateFirmware">
            <span>{{ getFileNames(files) }}</span>
          </v-text-field-file>
        </div>
        <v-button color="" class="min-w-[100px]" size="small" :disabled="isDisabledFirmware" @click="onSureFlash('firmware')">Update</v-button>
      </div>

      <div class="flex items-center gap-4 mb-4">
        <div class="md:flex flex-auto gap-4">
          <h6 class="text-gray-600 bg:text-gray-400">LittleFS:</h6>

          <v-text-field-file v-slot="{ files }" accept=".bin" @change="onUpdateLittleFS">
            <span>{{ getFileNames(files) }}</span>
          </v-text-field-file>
        </div>

        <v-button class="min-w-[100px]" size="small" :disabled="isDisabledLittleFS" @click="onSureFlash('LittleFS')">Update</v-button>
      </div>
    </VCardGray>

    <VCardGray title="System">
      <div class="flex items-center mb-4">
        <div class="flex-auto text-gray-600 bg:text-gray-400">Reboot device</div>

        <v-button class="min-w-[100px]" size="small" @click="emit('reboot', $event)">Reboot</v-button>
      </div>

      <div class="flex items-center mb-4">
        <div class="flex-auto text-gray-600 bg:text-gray-400">Reset configuration</div>

        <v-button class="min-w-[100px]" size="small" @click="emit('reset', $event)">Reset</v-button>
      </div>
    </VCardGray>
  </div>
</template>

<script setup lang="ts">
import { computed, ref, defineEmits, inject, nextTick } from 'vue';

import type { TypeTextFieldFile, TypeTextFieldEvent } from '@/types/types.ts';

import { DialogKey } from '@/simbol/index.ts';

const emit = defineEmits<{
  (e: 'reset', value: Event): void;
  (e: 'reboot', value: Event): void;
}>();

const dialog = inject(DialogKey, ({}) => {});

const data = { LittleFS: null, firmware: null };

const selectFile = ref<any>(data);

const onUpdateFirmware = (e: TypeTextFieldEvent) => (selectFile.value.firmware = e);
const onUpdateLittleFS = (e: TypeTextFieldEvent) => (selectFile.value.LittleFS = e);
const isDisabledFirmware = computed(() => Boolean(!selectFile.value?.firmware));
const isDisabledLittleFS = computed(() => Boolean(!selectFile.value?.LittleFS));

const getFileNames = (files: TypeTextFieldFile[]) => (files.length ? files.map((i) => `${i.name} (${i.size}) Byte`).join('') : 'Select a file...');
const getName = (name: string) => (selectFile.value?.[name]?.info?.files || []).map((i: TypeTextFieldFile) => `File: ${i.name} <br/> Size: ${i.size} B`).join('');

const onFlash = async (name: string) => {
  if (!selectFile.value?.[name]) return;
  const { files } = selectFile.value[name];
  const date = new FormData();
  for (let i = 0; i < files.length; i++) {
    const file = files.item(i);
    date.append(`file[${i}]`, file, `${name}.bin`);
  }
  if (!files.length) return;
  const res = await (await fetch('/update', { method: 'POST', body: date })).json();
  if (res?.state) dialog({ value: true, title: 'Done', message: 'Reboot...' });
};

const updateFirmware = () => nextTick(() => onFlash('firmware'));
const updateLittleFS = () => nextTick(() => onFlash('LittleFS'));

const onSureFlash = (name: string) =>
  dialog({
    value: true,
    message: `Are you sure you want to update the ${name}? <br/> <p class="mt-2" >${getName(name)}</p>`,
    callback: name === 'firmware' ? updateFirmware : updateLittleFS,
  });
</script>
