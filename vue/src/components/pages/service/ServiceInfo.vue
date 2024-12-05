<template>
  <div class="file-info">
    <div class="file-info__body my-6 mb-10">
      <v-progressbar :value="procent"></v-progressbar>
    </div>
    <div class="file-info__footer flex justify-between">
      <div v-for="(item, key) in date" :key="key" class="center">
        <div class="text-title-1 grey-base">{{ key }}</div>
        <div class="text-h6">{{ toByte(item) }}</div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { computed, defineProps } from 'vue';
import { toByte } from '@/utils/func/';
const props = defineProps({
  totalBytes: { type: Number, default: 0 },
  usedBytes: { type: Number, default: 0 },
});

const date = computed(() => ({ Used: props.usedBytes, Available: props.totalBytes - props.usedBytes, Total: props.totalBytes }));
const procent = computed(() => (props.usedBytes * 100) / props.totalBytes);
</script>

<style lang="scss">
.file-info {
  max-width: 100%;
  position: relative;
  background: var(--bg-2);
  border-radius: 10px;
  padding: 20px;

  @include above($sm) {
    max-width: 320px;
  }
}
</style>
