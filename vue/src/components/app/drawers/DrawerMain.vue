<template>
  <div class="drawer-main">
    <div class="drawer-main__menu mt-6">
      <v-main-menu @route="onClose" />
    </div>
    <div class="v-spacer"></div>
    <div class="drawer-main__btn mb-6">
      <div>
        Chip ID:
        <span class="grey-base">{{ id.toString(16) }}</span>
      </div>
      <div>
        Frimware:
        <span class="grey-base">{{ frimware }}</span>
      </div>
      <div>
        Total Bytes:
        <span class="grey-base">{{ totalBytes }}</span>
      </div>
      <div>
        Used Bytes:
        <span class="grey-base">{{ usedBytes }}</span>
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

const frimware = computed(() => props.info?.frimware || '');
const id = computed(() => props.info?.id || '');
const totalBytes = computed(() => props.info?.totalBytes || '');
const usedBytes = computed(() => props.info?.usedBytes || '');
const onClose = value => emit('close', value);
</script>

<style lang="scss">
.drawer-main {
  display: flex;
  flex-direction: column;
  height: 100%;
  &__btn {
    display: flex;
    flex-direction: column;
    align-items: center;
  }
  &__menu {
    .v-main-menu {
      flex-direction: column;
    }
    .v-main-menu__item {
      font-size: 24px;
    }
  }
}
</style>
