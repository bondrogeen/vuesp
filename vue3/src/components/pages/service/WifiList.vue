<template>
  <div class="list-wifi">
    <v-list v-slot="{ item }" :list="list">
      <div class="d-flex align-center w-100" @click="onSelect(item)">
        <div class="mr-2"><WifiIcon v-bind="item" /></div>
        <div>
          <div class="text-title1">{{ item.ssid }}</div>
          <div class="text-body-2 grey-base">Security: {{ listEncryption[item.encryptionType] || 'unknown' }}</div>
        </div>
      </div>
    </v-list>
    <div v-if="!list.length">
      <v-loader></v-loader>
    </div>
  </div>
</template>

<script setup>
import { defineProps, defineEmits } from 'vue';

defineProps({
  list: { type: Array, default: () => [] },
});

const emit = defineEmits(['select']);
const listEncryption = ['OPEN', 'WEP', 'WPA_PSK', 'WPA2_PSK', 'WPA_WPA2_PSK', 'MAX', '', 'NO', 'AUTO'];
const onSelect = e => emit('select', e);
</script>

<style lang="scss">
.list-wifi {
  width: 100%;
  border-bottom: 1px solid color('grey', 'lighten-1');
}
</style>
