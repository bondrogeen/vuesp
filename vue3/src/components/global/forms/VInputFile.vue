<template>
  <label class="v-input-file">
    <input type="file" multiple @change="onChange" />
    <slot></slot>
  </label>
</template>

<script setup>
import { computed, defineProps, defineEmits } from 'vue';
const props = defineProps({
  value: { type: String, default: '' },
  path: { type: String, default: '/' },
  info: { type: Object, default: () => ({}) },
});

const emit = defineEmits(['submit', 'error']);

const availableByte = computed(() => props.info.totalBytes - props.info.usedBytes);

const onChange = async e => {
  const formData = new FormData();
  const files = e.target.files;
  let totalSize = 0;

  for (let i = 0; i < files.length; i++) {
    const file = files.item(i);
    totalSize += file.size;
    const fileName = `${props.path}${file.name}`;
    formData.append(`file[${i}]`, file, fileName);
  }
  if (!files.length) return;
  if (totalSize < availableByte.value) {
    emit('submit', formData);
  } else {
    emit('error', { value: true, message: 'No free space on filesystem' });
  }
};
</script>

<style lang="scss">
.v-input-file {
  position: relative;
  input[type='file'] {
    display: none;
  }
}
</style>
