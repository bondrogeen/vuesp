<template>
  <div class="tabs">
    <ul class="tabs__header">
      <li v-for="(tab, i) in tabs" :key="tab.title" @click="selectTab(i)">{{ tab.title }}</li>
    </ul>
    {{ tabs }}
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
    console.log(slots.default())
      if (slots.default) {
        tabs.value = slots.default().filter((child) => child.type.__name === "VTab");
      }
    });

const selectTab = i => {
  selectedIndex.value = i;
  // loop over all the tabs
  tabs.value.forEach((tab, index) => {
    tab.isActive = index === i;
  });
};
</script>
