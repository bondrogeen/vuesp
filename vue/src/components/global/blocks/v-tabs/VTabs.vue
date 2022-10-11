<template>
  <div class="v-tabs">
    <ul class="v-tabs__header text-title-1 scroll-none">
      <li v-for="item of tabs" :key="item.label" :class="['v-tabs__item', { 'v-tabs__item--active': isActive(item.label) }]" @click="onSelect(item)">
        <slot name="icon" :item="item">
          <v-icons v-if="item.icon" class="v-tabs__icon" :icon="item.icon"></v-icons>
        </slot>
        <div class="v-tabs__title">
          {{ item.label }}
        </div>
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

const selectedIndex = ref({});
const tabs = ref([]);

provide('selected', selectedIndex);

const isActive = label => selectedIndex.value.label === label;

const onSelect = i => {
  selectedIndex.value = i;
  emit('change', i);
};

onBeforeMount(() => {
  if (slots.default) {
    tabs.value = slots
      .default()
      .filter(child => child.type.__name === 'VTab')
      .map(i => i.props);
    selectedIndex.value = tabs.value[0];
    console.log(selectedIndex.value);
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
    border-bottom: 1px solid var(--border-1);
    display: flex;
    align-items: center;
    justify-content: space-between;
    @include above($sm) {
      justify-content: flex-start;
    }
  }
  &__icon {
    height: 34px;
    width: 34px;
    @include above($sm) {
      height: 24px;
      width: 24px;
    }
    color: var(--label-1);
  }
  &__title {
    display: none;
    @include above($sm) {
      display: block;
    }
  }
  &__item {
    cursor: pointer;
    user-select: none;
    position: relative;
    padding: 0 20px;
    display: inline-flex;
    align-items: center;
    height: 48px;
    margin: 0;
    gap: 10px;
    @include above($sm) {
      padding: 0 15px;
    }
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
