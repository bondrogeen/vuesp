import type { Ref } from 'vue';
import type { IGpio } from 'vuesp-components/types';

import { ref } from 'vue';
import { getBinary, onUploadBinary } from '@/utils/fs';
import { struct } from '@/utils/struct.ts';

import { command, parseDateGPIO, stringifyDateGPIO } from '@/utils/gpio';

import { pathGPIO } from '@/utils/const.ts';

export const usePorts = (onSend: (data: any) => void) => {
  interface TypeMode {
    name: string;
    value: number;
  }

  const ports: Ref<Record<string | number, IGpio>> = ref({});

  const enum GPIOMode {
    INPUT,
    OUTPUT,
    INPUT_PULLUP,
    OUTPUT_OPEN_DRAIN,
  }

  const listMode: TypeMode[] = [
    { name: 'INPUT', value: 0 }, // 0x00
    { name: 'INPUT_PULLUP', value: 2 }, // 0x02
    { name: 'OUTPUT', value: 1 }, // 0x01
    { name: 'OUTPUT_OPEN_DRAIN', value: 3 }, // 0x03
    // { name: 'INPUT_PULLDOWN_16', value: 12 }, // 0x04
    // { name: 'WAKEUP_PULLUP', value: 13 }, // 0x05
    // { name: 'WAKEUP_PULLDOWN', value: 15 }, // 0x07
  ];

  const listInterrupt: TypeMode[] = [
    { name: 'OFF', value: 0 }, // 0x00
    { name: 'RISING', value: 1 }, // 0x01
    { name: 'FALLING', value: 2 }, // 0x02
    { name: 'CHANGE', value: 3 }, // 0x03
  ];

  const isOutput = ({ mode }: IGpio) => [GPIOMode.OUTPUT, GPIOMode.OUTPUT_OPEN_DRAIN].includes(mode);
  const isInput = ({ mode }: IGpio) => [GPIOMode.INPUT, GPIOMode.INPUT_PULLUP].includes(mode);

  const onMode = ({ gpio }: IGpio, { value }: TypeMode) => (ports.value[gpio].mode = value);
  const onInputValue = ({ gpio }: IGpio, { value }: TypeMode) => (ports.value[gpio].mode = value);

  const onInterrupt = ({ gpio }: IGpio, { value }: TypeMode) => {
    ports.value[gpio].interrupt = value;
  };

  const getStateValue = (gpio: Record<string, IGpio>, port: IGpio) => gpio?.[port.gpio]?.value;

  const onSetPort = (port: IGpio, value: number) => {
    onSend({ comm: 'PORT', data: { ...port, command: command.GPIO_COMMAND_SET, value } });
  };

  const onGetPort = () => {
    onSend({ comm: 'PORT', data: { command: command.GPIO_COMMAND_GET_ALL } });
  };

  const onLoadDataGpio = async (): Promise<Record<string, IGpio>> => {
    const array = await getBinary(pathGPIO);
    const data: ArrayBuffer[] = parseDateGPIO(array);

    return data.reduce<Record<string, IGpio>>((acc, i) => {
      const { object } = (struct.get(i) || {}) as { object: IGpio };
      const gpio = object.gpio;
      acc[gpio] = object;
      return acc;
    }, {});
  };

  const init = async () => {
    ports.value = await onLoadDataGpio();
    onGetPort();
  };

  return { ports, listMode, listInterrupt, init, getStateValue, onSetPort, onMode, onInterrupt, onInputValue, onUploadBinary, isOutput, isInput, stringifyDateGPIO };
};
