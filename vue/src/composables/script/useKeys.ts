import { useMagicKeys, whenever } from '@vueuse/core';

export const useKeys = () => {
  const { ctrl_s, ctrl_f } = useMagicKeys({
    passive: false,
    onEventFired(e) {
      if (e.ctrlKey && e.key === 's' && e.type === 'keydown') e.preventDefault();
      if (e.ctrlKey && e.key === 'f' && e.type === 'keydown') e.preventDefault();
    },
  });

  whenever(ctrl_s, () => {
    // onSave();
  });
  whenever(ctrl_f, () => {
    // if (editor.value) editor.value.update({ value: formatScript(editor.value.value) });
  });
  return {

  };
};
