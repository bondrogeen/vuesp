<template>
  <card-base v-bind="props" @click="onClick" size="lg" @open="openDialog">
    <template #value>{{ getValue }} °C</template>
    <template #dialog-value>Average {{ getValue }} °C</template>

    <template #dialog>
      <div class="w-full overflow-x-auto scrollbar">
        <div class="h-[300px] min-w-10" :style="{ width: dataLength * 30 + 'px' }">
          <canvas :id="getID(props)" height="300" width="0"></canvas>
        </div>
      </div>
      <v-button class="mt-4" @click="onUpdate">{{ $t('btnUpdate') }}</v-button>
    </template>
  </card-base>
</template>

<script setup lang="ts">
import { defineEmits, defineProps, computed } from 'vue';

import type { IDashboardItem, IMessageBuffer } from '@/types/';

import { loadScript, timeUtcToString } from 'vuesp-components/helpers';

const emit = defineEmits<{
  (e: 'click', event: Event): void;
  (e: 'setState', item: any): void;
  (e: 'update', event: Event): void;
}>();

interface Props extends Omit<IDashboardItem, 'value'> {
  value?: IMessageBuffer;
}

const props = defineProps<Props>();

declare global {
  interface Window {
    Chart: any;
  }
}

const getID = ({ id }: Props): string => `chart-` + id.replace('.', '_');

const onClick = (event: Event) => emit('click', event);

const data = computed(() => {
  const { data = [], head = 0, count = 0, tail = 0 } = props.value || {};
  if (count === 0) return [];

  const result = [];
  if (head <= tail) {
    for (let i = tail; i <= count - 1; i++) result.push(data[i]);
    for (let i = 0; i <= head - 1; i++) result.push(data[i]);
  } else {
    for (let i = tail; i <= head - 1; i++) result.push(data[i]);
  }
  return result;
});

const dataLength = computed(() => data.value.length);
const getValue = computed(() => (data.value.reduce((acc: number, i: number) => acc + i, 0) / dataLength.value).toFixed(2));

const getTime = (i: number) => new Date(Date.now() - (dataLength.value - i) * 10000);

let chart: any;

const createChart = () => {
  const el = document.querySelector(`#${getID(props)}`);
  if (!el) return;
  const Chart = window.Chart;
  chart = new Chart(el, {
    type: 'line',
    data: {
      labels: new Array(dataLength.value).fill(0).map((_, i) => timeUtcToString(getTime(i), { minute: '2-digit', hour: '2-digit', second: '2-digit' })),
      datasets: [{ label: 'Temp', data: data.value, borderWidth: 1 }],
    },
    options: {
      scales: { x: { display: true }, y: { display: true } },
      animations: {
        radius: { duration: 400, easing: 'linear', loop: (context: any) => context.active },
      },
      hoverRadius: 6,
      hoverBackgroundColor: 'blue',
      interaction: {
        mode: 'nearest',
        intersect: false,
        axis: 'x',
      },
    },
  });
};

const onUpdate = (e: Event) => {
  if (chart) {
    chart.destroy();
    emit('update', e);
    createChart();
  }
};

const openDialog = async () => {
  // https://www.chartjs.org/
  await loadScript('https://cdn.jsdelivr.net/npm/chart.js');
  createChart();
};
</script>
