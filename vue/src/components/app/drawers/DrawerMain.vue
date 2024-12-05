<template>
  <div class="h-100 drawer-main flex f-column">
    <div class="drawer-main__menu v-spacer mt-6">
      <v-main-menu class="flex items-center f-column gap-6 text-h3" @route="onClose" />
    </div>
    <div class="flex items-center f-column mb-6">
      <div v-for="(item, key) in data" :key="key">
        {{ key }}:
        <span class="grey-base">{{ item }}</span>
      </div>
    </div>
  </div>
</template>

<script setup>
import { computed, defineEmits, defineProps } from 'vue';
const props = defineProps({
  state: { type: Boolean, default: false },
  info: { type: Object, default: () => {} },
});
const emit = defineEmits(['close']);

const data = computed(() => ({
  Frimware: (props.info?.firmware || []).join('.'),
  'Chip ID': (props.info?.id || '').toString(16),
  'Total Bytes': props.info?.totalBytes || '',
  'Used Bytes': props.info?.usedBytes || '',
}));
const onClose = value => emit('close', value);
</script>
