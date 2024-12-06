<template>
  <div class="v-tabs">
    <ul class="v-tabs__header w-full flex border-b border-gray-200 no-scrollbar overflow-y-hidden overflow-x-auto">
      <li v-for="item of tabs" :key="item.label" class="relative transition duration-500 ease-in-out" :class="['v-tabs__item', { 'v-tabs__item--active ': isActive(item.label) }]">
        <button class="p-4 font-bold" @click="onSelect(item)">{{ item.label }}</button>
      </li>
    </ul>
    <div class="">
      <slot></slot>
    </div>
  </div>
</template>

<script setup>
import { ref, defineProps, defineEmits, useSlots, provide, onMounted } from 'vue';
defineProps({
  value: { type: Boolean, default: false },
});
const emit = defineEmits(['change']);

const slots = useSlots();

const selectedIndex = ref({});
const tabs = ref([]);

provide('selected', selectedIndex);

const isActive = label => selectedIndex.value?.label === label;

const onSelect = i => {
  selectedIndex.value = i;
  emit('change', i);
};

onMounted(() => {
  if (slots.default) {
    setTimeout(() => {
      tabs.value = slots
        .default()
        .filter(child => child.type.__name === 'VTab')
        .map(i => i.props);
      selectedIndex.value = tabs.value?.[0] || {};
    }, 300);
  }
});
</script>

<style lang="scss">
.v-tabs {
  &__item {
    user-select: none;

    &::before,
    &::after {
      content: '';
      position: absolute;
      height: 2px;
      width: 0;
      bottom: 0;
      transition: all 0.2s ease-in-out;
      background-color: color('app', 'primary');
    }

    &::before {
      right: 50%;
    }
    &::after {
      left: 50%;
    }
    &--active {
      &::before,
      &::after {
        color: color('app', 'primary');
        width: 50%;
      }
    }
  }
}
</style>
