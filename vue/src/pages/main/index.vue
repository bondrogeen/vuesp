<template>
  <div class="container">
    <div class="row">
      <h1 class="col sm12 text-h2 mb-6">Main</h1>
      <div class="col sm12 lg8 xl7">
        <v-tabs>
          <v-tab label="Sensors" icon="connect">
            <div class="row">
              <div class="col sm12">
                <div v-for="(sensor, key) in getSensors" :key="key">
                  <h4 class="mb-4 text-h4">{{ key }}</h4>
                  <ul class="mb-4">
                    <li v-for="(item, name) in sensor" :key="name">
                      <span class="grey-base text-up">{{ name }}</span>
                      : {{ getValue(item) }}
                    </li>
                  </ul>
                </div>
              </div>
            </div>
          </v-tab>
          <v-tab label="Paint" icon="onher">
            <div class="row">
              <!-- <MainPaint class="col sm12" /> -->
            </div>
          </v-tab>
        </v-tabs>
      </div>
    </div>
  </div>
</template>

<script setup>
import { computed, defineProps } from 'vue';
import { storeToRefs } from 'pinia';
import { useWebSocketStore } from '@/stores/WebSocketStore';

// import MainPaint from '@/components/pages/main/MainPaint';

// import event from '@/assets/js/event';

const props = defineProps({
  changeTheme: { type: Function, default: () => {} },
});

props.changeTheme(true);

const webSocketStore = useWebSocketStore();
const { sensors } = storeToRefs(webSocketStore);

const getSensors = computed(() => ({
  BMP280: {
    temperature: sensors.value.bmpTemperature,
    pressure: sensors.value.bmpPressure,
    altitude: sensors.value.bmpAltitude,
  },
  AHT15: {
    temperature: sensors.value.ahtTemperature,
    humidity: sensors.value.ahtHumidity,
  },
}));

const getValue = (value) => (value ? value.toFixed(2) : '--');
</script>
