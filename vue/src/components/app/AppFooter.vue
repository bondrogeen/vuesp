<template>
  <footer class="py-4 px-4 bg-blue-50">
    <div class="flex items-center justify-between container mx-auto">
      <div class="md:flex items-center flex-wrap gap-4">
        <div>© 2021 - {{ new Date().getFullYear() }} Vuesp.</div>

        <div>
          Firmware:
          <span class="text-gray-400 text-body">(ver.{{ getFirmware }})</span>
        </div>

        <div>
          ID:
          <span class="text-gray-400 text-body">{{ id.toString(16) }}</span>
        </div>
      </div>

      <div>
        <a href="https://github.com/bondrogeen/vuesp" target="_blank">GitHub</a>
      </div>
    </div>
  </footer>
</template>

<script setup>
import { computed, defineProps } from 'vue';
const props = defineProps({
  firmware: { type: Array, default: () => [] },
  id: { type: Number, default: 0 },
});

const getFirmware = computed(() => props.firmware.join('.'));
</script>
