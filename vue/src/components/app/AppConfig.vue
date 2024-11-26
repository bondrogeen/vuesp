<template>
  <div>
    <v-tabs>
      <v-tab v-for="(item, key) in config" :key="key" label="key">
        1
        <div v-for="field of item" :key="field.name">
          <v-input :modelValue="field.name" :label="field.name" />
        </div>
      </v-tab>
    </v-tabs>
  </div>
</template>

<script setup>
import { ref, defineProps, defineEmits } from 'vue';

const props = defineProps({
  config: { type: Object, default: () => ({}) },
  path: { type: Boolean, default: false },
});

const emit = defineEmits(['submit']);

const form = ref({});

const onClick = () => emit('submit', props.config);
</script>

<style lang="scss">
.app-dialog {
}
</style>
