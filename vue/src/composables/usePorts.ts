import type { Ref } from 'vue';
import type { IGpio, IStateGpio } from '@/utils/types/types.ts';

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

  const ports: Ref<IGpio[]> = ref([]);
  const portsDef = ref([]);

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

  const onMode = (port: IGpio, item: TypeMode) => {
    console.log(port, item);

    // const obj = getData(port.data) || {};
    // const value = item?.value || 0;
    // obj.init = value & 0b00001111 ? 1 : 0;
    // obj.mode = value & 0b00000111;
    // port.data = setData(obj);
  };

  const onInterrupt = (port: IGpio, item: TypeMode) => {
    console.log(port, item);
    // const obj = getData(port.data) || {};
    // console.log(obj);
    // const value = item?.value || 0;
    // obj.interrupt = value & 0b00000111;
    // port.data = setData(obj);
  };

  // const getMode = ({ mode }: IGpio): TypeMode | undefined => {
  //   const mode = getKey(data, 'mode');
  //   return listMode.find((i) => i.value === mode);
  // };

  const getStateValue = (gpio: IStateGpio, port: IGpio) => gpio?.[port.gpio]?.value;

  // const isDisabled = (pin: IGpio) => Boolean(![9, 11].includes(getMode(pin)?.value || 0));

  const onSetPort = (port: IGpio, value: number) => {
    onSend({ comm: 'PORT', data: { ...port, command: command.GPIO_COMMAND_SET, value } });
  };

  const onGetPort = () => {
    onSend({ comm: 'PORT', data: { command: command.GPIO_COMMAND_GET_ALL } });
  };

  const onLoadDataGpio = async (): Promise<IGpio[]> => {
    const array = await getBinary(pathGPIO);
    const data: ArrayBuffer[] = parseDateGPIO(array);
    return data.map((i) => {
      const { object } = (struct.get(i) || {}) as { object: IGpio };
      return object;
    });
  };

  const init = async () => {
    ports.value = await onLoadDataGpio();
    // portsDef.value = JSON.parse(JSON.stringify(ports.value));
    onGetPort();
  };

  return { ports, portsDef, listMode, listInterrupt, init, getStateValue, onSetPort, onMode, onInterrupt, onUploadBinary, stringifyDateGPIO };
};
