<template>
  <div class="v-tabs">
    <ul class="v-tabs__header text-title-1">
      <li
        v-for="(tab, i) in tabs"
        :key="tab.props.label"
        class="v-tabs__item"
        :class="{ 'v-tabs__item--active': selectedIndex === i }"
        @click="selectTab(i)"
      >
        {{ tab.props.label }}
      </li>
    </ul>
    <slot></slot>
  </div>
</template>

<script setup>
import { ref, defineProps, defineEmits, onBeforeMount, useSlots } from 'vue';
const slots = useSlots();
defineEmits(['close']);
defineProps({
  value: { type: Boolean, default: false },
});

const selectedIndex = ref(0);
const tabs = ref([]);

onBeforeMount(() => {
  console.log(slots.default());
  if (slots.default) {
    tabs.value = slots.default().filter(child => child.type.__name === 'VTab');
  }
});

const selectTab = i => {
  selectedIndex.value = i;
  tabs.value.forEach((tab, index) => {
    console.log(tab, index);
    // tab.isActive = index === i;
  });
};
</script>

<style lang="scss">
.v-tabs {
  box-shadow: 0 2px 2px 0 rgb(0 0 0 / 14%), 0 3px 1px -2px rgb(0 0 0 / 12%), 0 1px 5px 0 rgb(0 0 0 / 20%);
  &__header {
    position: relative;
    overflow-x: auto;
    overflow-y: hidden;
    height: 48px;
    width: 100%;
    background-color: color('white', 'base');
    margin: 0 auto;
    white-space: nowrap;
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
    // text-transform: uppercase;

    &::before {
      right: 50%;
    }
    &::after {
      left: 50%;
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
    &--active {
      &::before,
      &::after {
        width: 50%;
      }
    }
  }
}
</style>
