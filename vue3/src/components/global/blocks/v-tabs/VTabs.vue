<template>
  <div class="v-tabs">
    <ul class="v-tabs__header text-title-1 scroll-none">
      <li v-for="tab of tabs" :key="tab" :class="['v-tabs__item', { 'v-tabs__item--active': selectedIndex === tab }]" @click="onSelect(tab)">
        {{ tab }}
      </li>
    </ul>
    <div class="v-tabs__content">
      <slot></slot>
    </div>
  </div>
</template>

<script setup>
import { ref, defineProps, defineEmits, onBeforeMount, useSlots, provide } from 'vue';
defineProps({
  value: { type: Boolean, default: false },
});
const emit = defineEmits(['change']);

const slots = useSlots();

const selectedIndex = ref(0);
const tabs = ref([]);

provide('selected', selectedIndex);

const onSelect = i => {
  selectedIndex.value = i;
  emit('change', i);
};

onBeforeMount(() => {
  if (slots.default) {
    tabs.value = slots
      .default()
      .filter(child => child.type.__name === 'VTab')
      .map(i => i.props.label);
    selectedIndex.value = tabs.value[0];
  }
});
</script>

<style lang="scss">
.v-tabs {
  &__header {
    position: relative;
    overflow-x: auto;
    overflow-y: hidden;
    height: 48px;
    width: 100%;
    // background-color: color('white', 'base');
    margin: 0 auto;
    white-space: nowrap;
    font-weight: 600;
    // box-shadow: 0 2px 2px 0 rgb(0 0 0 / 14%), 0 3px 1px -2px rgb(0 0 0 / 12%), 0 1px 5px 0 rgb(0 0 0 / 20%);
  }
  &__item {
    cursor: pointer;
    user-select: none;
    position: relative;
    padding: 0 15px;
    display: inline-block;
    text-align: center;
    line-height: 48px;
    height: 48px;
    margin: 0;
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
        width: 50%;
      }
    }
  }
  &__content {
    padding: 30px 0;
  }
}
</style>
