<template>
  <ul class="bg-white">
    <li
      v-for="(item, i) of list"
      :key="'list_' + i"
      class="whitespace-nowrap cursor-pointer flex items-center min-h-[40px] py-1 px-4 border-b last:border-b-0 hover:bg-blue-50"
      @click="onClick(item)"
    >
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
