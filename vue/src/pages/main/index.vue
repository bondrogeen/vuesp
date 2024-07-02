<template>
  <div class="container">
    <div class="row">
      <h1 class="col sm12 text-h2 mb-6">Service</h1>
      <div class="col sm12 lg8 xl7">
        <v-tabs>
          <v-tab label="Sensors" icon="connect">
            <div class="row">
              <div class="col sm12">
                <ul>
                  <li v-for="(item, key) in sensors" :key="key">{{ key }}: {{ item }}</li>
                </ul>
              </div>
            </div>
          </v-tab>
          <v-tab label="Paint" icon="onher">
            <div class="row">
              <MainPaint />
            </div>
          </v-tab>
        </v-tabs>
      </div>
    </div>
  </div>
</template>

<script setup>
import { defineProps } from 'vue';
import { storeToRefs } from 'pinia';
import { useWebSocketStore } from '@/stores/WebSocketStore';

import MainPaint from '@/components/pages/main/MainPaint';

// import event from '@/assets/js/event';

const props = defineProps({
  changeTheme: { type: Function, default: () => {} },
});

props.changeTheme(true);

const webSocketStore = useWebSocketStore();
const { sensors } = storeToRefs(webSocketStore);
</script>

<style lang="scss"></style>
