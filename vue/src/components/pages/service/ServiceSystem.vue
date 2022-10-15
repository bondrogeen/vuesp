<template>
  <div class="service-update">
    <div class="text-h5 mb-2">Update</div>
    <div class="mb-4 d-flex a-center">
      <div class="service-update__title text-title-1">Frimware:</div>
      <v-input-file v-slot="{ files }" accept=".bin" @change="onUpdateFrimware">
        <span class="grey-base">{{ getFileNames(files) }}</span>
      </v-input-file>
      <div class="v-spacer"></div>
      <v-button size="small" :disabled="isDisabledFrimware" @click="onSureFlash('frimware')">Update</v-button>
    </div>
    <div class="d-flex a-center mb-4">
      <div class="service-update__title text-title-1">Littlefs:</div>
      <v-input-file v-slot="{ files }" accept=".bin" @change="onUpdateLittlefs">
        <span class="grey-base">{{ getFileNames(files) }}</span>
      </v-input-file>
      <div class="v-spacer"></div>
      <v-button size="small" :disabled="isDisabledLittlefs" @click="onSureFlash('littlefs')">Update</v-button>
    </div>
    <div class="text-h5 mb-2">Reboot</div>
    <div class="d-flex a-center mb-4">
      <div class="v-spacer grey-base">Reboot device</div>
      <v-button size="small" @click="onEmit('reboot')">Reboot</v-button>
    </div>
    <div class="text-h5 mb-2">Reset</div>
    <div class="d-flex a-center mb-4">
      <div class="v-spacer grey-base">Reset configuration</div>
      <v-button size="small" @click="onEmit('reset')">Reset</v-button>
    </div>
  </div>
</template>

<script setup>
import { computed, ref, defineEmits, inject, nextTick } from 'vue';

const emit = defineEmits(['done', 'reboot', 'reset']);
const dialog = inject('dialog');

const selectFile = ref({ littlefs: null, frimware: null });
const onUpdateFrimware = e => (selectFile.value.frimware = e);
const onUpdateLittlefs = e => (selectFile.value.littlefs = e);
const isDisabledFrimware = computed(() => Boolean(!selectFile.value?.frimware));
const isDisabledLittlefs = computed(() => Boolean(!selectFile.value?.littlefs));

const getFileNames = files => (files.length ? files.map(i => `${i.name} (${i.size}) Byte`).join('') : 'Select a file...');
const getName = name => (selectFile.value?.[name]?.info?.files || []).map(i => `File: ${i.name} <br/> Size: ${i.size} B`).join('');

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
  if (res?.state) dialog({ title: 'Done', message: 'Reboot...' });
};

const updateFirmware = () => nextTick(() => onFlash('frimware'));
const updateLittlefs = () => nextTick(() => onFlash('littlefs'));

const onSureFlash = name =>
  dialog({
    message: `Are you sure you want to update the ${name}?. <br/> <p class="grey-base text-body-1 mt-2" >${getName(name)}</p>`,
    callback: name === 'frimware' ? updateFirmware : updateLittlefs,
  });

const onEmit = name => emit(name);
</script>

<style lang="scss">
.service-update {
  &__title {
    min-width: 100px;
  }
}
</style>
