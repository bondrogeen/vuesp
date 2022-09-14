<template>
  <v-card class="file-info">
    <div class="file-info__body my-6 mb-10">
      <v-progressbar :value="procent"></v-progressbar>
    </div>
    <div class="file-info__footer d-flex justify-between">
      <div class="center">
        <div class="text-title-1 grey-base">Used</div>
        <div class="text-h5">{{ toByte(usedBytes) }}</div>
      </div>
      <div class="center">
        <div class="text-title-1 grey-base">Available</div>
        <div class="text-h5">{{ toByte(availableByte) }}</div>
      </div>
      <div class="center">
        <div class="text-title-1 grey-base">Total</div>
        <div class="text-h5">{{ toByte(totalBytes) }}</div>
      </div>
    </div>
  </v-card>
</template>

<script setup>
import { computed, defineProps } from 'vue';
import { toByte } from '@/utils/func/';
const props = defineProps({
  totalBytes: { type: Number, default: 0 },
  usedBytes: { type: Number, default: 0 },
});
const availableByte = computed(() => props.totalBytes - props.usedBytes);
const procent = computed(() => (props.usedBytes * 100) / props.totalBytes);
</script>

<style lang="scss">
.file-info {
  &__footer {
  }
}
</style>
