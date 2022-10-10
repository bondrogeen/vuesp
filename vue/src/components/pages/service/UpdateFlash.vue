<template>
  <div>
    <div class="text-h5 mb-4">Frimware</div>
    <div class="mb-6 d-flex align-center">
      <v-input-file v-slot="{ files }" accept=".bin" @change="onUpdateFrimware">
        <span class="grey-base">{{ getFileNames(files) }}</span>
      </v-input-file>
      <div class="v-spacer"></div>
      <v-button size="small" :disabled="isDisabledFrimware" @click="onFlash('frimware')">Update</v-button>
    </div>
    <div class="text-h5 mb-4">Littlefs:</div>
    <div class="mb-6 d-flex align-center">
      <v-input-file v-slot="{ files }" accept=".bin" @change="onUpdateLittlefs">
        <span class="grey-base">{{ getFileNames(files) }}</span>
      </v-input-file>
      <div class="v-spacer"></div>
      <v-button size="small" :disabled="isDisabledLittlefs" @click="onFlash('littlefs')">Update</v-button>
    </div>
  </div>
</template>

<script setup>
import { computed, ref, defineEmits } from 'vue';

const emit = defineEmits(['done']);

const selectFile = ref({ littlefs: null, frimware: null });
const onUpdateFrimware = e => (selectFile.value.frimware = e);
const onUpdateLittlefs = e => (selectFile.value.littlefs = e);
const isDisabledFrimware = computed(() => Boolean(!selectFile.value?.frimware));
const isDisabledLittlefs = computed(() => Boolean(!selectFile.value?.littlefs));

const getFileNames = files => (files.length ? files.map(i => `${i.name} (${i.size}) Byte`).join('') : 'Select a file...');

const onFlash = async name => {
  if (!selectFile.value?.[name]) return;
  const { files } = selectFile.value[name];
  const date = new FormData();
  for (let i = 0; i < files.length; i++) {
    const file = files.item(i);
    date.append(`file[${i}]`, file, `${name}.bin`);
  }
  if (!files.length) return;
  const res = await (await fetch('/update', { method: 'POST', body: date })).json();
  emit('done', res);
};
</script>
