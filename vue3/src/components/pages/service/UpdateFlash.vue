<template>
  <div class="row">
    <div class="col sm12 md7 mb-6 d-flex align-center">
      <div class="text-title-1 mr-4">Frimware:</div>
      <v-input-file v-slot="{ files }" accept=".bin" @change="onUpdate">
        <span class="grey-base">{{ getFileNames(files) }}</span>
      </v-input-file>
      <div class="v-spacer"></div>
      <v-button size="small" @click="onFlash">Update</v-button>
    </div>
    <div class="col sm12 md7 mb-6 d-flex align-center">
      <div class="text-title-1 mr-4">Littlefs:</div>
      <v-input-file v-slot="{ files }" accept=".bin" @change="onUpdate">
        <span class="grey-base">{{ getFileNames(files) }}</span>
      </v-input-file>
      <div class="v-spacer"></div>
      <v-button size="small" @click="onFlash">Update</v-button>
    </div>
  </div>
</template>

<script setup>
import { ref } from 'vue';

const selectFile = ref();
const onUpdate = e => {
  console.log(e);
  selectFile.value = e;
};

const onFlash = async () => {
  const { date } = selectFile.value
  console.log(date);
  const res = await (await fetch('/update', { method: 'POST', body: date })).json();
  if (res?.state) console.log(res);
};

const getFileNames = files => (files.length ? files.map(i => `${i.name} (${i.size}) Byte`).join('') : 'Select a file...');
</script>
