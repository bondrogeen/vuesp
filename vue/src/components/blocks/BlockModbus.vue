<template>
  <div>
    <VTextField class="col-span-3" hideMessage />
    <v-button block @click="onSend">Send</v-button>
  </div>
</template>

<script setup>
import { defineProps, defineEmits } from 'vue';

defineProps({
  modelValue: { type: [Boolean, Number], default: false },
  disabled: { type: Boolean, default: false },
});
const emit = defineEmits(['send']);

const onSend = () => emit('send', [0x01, 0x04, 0x00, 0x00, 0x00, 0x01, 0x31, 0xca]);
</script>
