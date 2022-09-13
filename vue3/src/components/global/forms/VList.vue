<template>
  <ul class="v-list">
    <li v-for="(item, i) of list" :key="'list_' + i" class="v-list__item" @click="onClick(item)">
      <slot :item="item">{{ item.name }}</slot>
    </li>
  </ul>
</template>

<script setup>
import { defineProps, defineEmits } from 'vue';
const emit = defineEmits(['click']);
defineProps({
  list: { type: Array, default: () => [] },
});

const onClick = e => emit('click', e);
</script>

<style lang="scss">
.v-list {
  background: #fff;
  border-radius: 0;
  list-style-type: none;
  padding: 0;
  margin: 0;
  &__item {
    height: 30px;
    padding: 0 15px;
    display: flex;
    align-items: center;
    cursor: pointer;
    &:hover {
      @media (hover: hover) {
        background-color: map-get($grey, 'lighten-1');
      }
    }

    white-space: nowrap;
    overflow: hidden;
  }
}
</style>
