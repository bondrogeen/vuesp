<template>
  <ul class="v-list bg-white">
    <li v-for="(item, i) of list" :key="'list_' + i" class="v-list__item px-4 border-b last:border-b-0" @click="onClick(item)">
      <slot :item="item">{{ item.name }}</slot>
    </li>
  </ul>
</template>

<script setup>
import { defineProps, defineEmits } from 'vue';
defineProps({
  list: { type: Array, default: () => [] },
});

const emit = defineEmits(['click']);
const onClick = e => emit('click', e);
</script>

<style lang="scss">
.v-list {
  &__item {
    min-height: 40px;
    display: flex;
    align-items: center;
    cursor: pointer;

    white-space: nowrap;
    &:hover {
      background-color: var(--hover);
    }
  }
}
</style>
