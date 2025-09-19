import type { Ref } from 'vue';
import type { IStoreGpio } from 'vuesp-components/types';

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

  const ports: Ref<Record<string | number, IStoreGpio>> = ref({});

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

  const isOutput = ({ mode }: IStoreGpio) => [GPIOMode.OUTPUT, GPIOMode.OUTPUT_OPEN_DRAIN].includes(mode);
  const isInput = ({ mode }: IStoreGpio) => [GPIOMode.INPUT, GPIOMode.INPUT_PULLUP].includes(mode);

  const onMode = ({ gpio }: IStoreGpio, { value }: TypeMode) => (ports.value[gpio].mode = value);
  const onInputValue = ({ gpio }: IStoreGpio, { value }: TypeMode) => (ports.value[gpio].mode = value);

  const onInterrupt = ({ gpio }: IStoreGpio, { value }: TypeMode) => {
    ports.value[gpio].interrupt = value;
  };

  const getStateValue = (gpio: Record<string, IStoreGpio>, port: IStoreGpio) => gpio?.[port.gpio]?.value;

  const onSetPort = (port: IStoreGpio, value: number) => {
    onSend({ comm: 'PORT', data: { ...port, command: command.GPIO_COMMAND_SET, value } });
  };

  const onGetPort = () => {
    onSend({ comm: 'PORT', data: { command: command.GPIO_COMMAND_GET_ALL } });
  };

  const onLoadDataGpio = async (): Promise<Record<string, IStoreGpio>> => {
    const array = await getBinary(pathGPIO);
    const data: ArrayBuffer[] = parseDateGPIO(array);

    return data.reduce<Record<string, IStoreGpio>>((acc, i) => {
      const { object } = (struct.get(i) || {}) as { object: IStoreGpio };
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
