import { useFetch } from '@vueuse/core';
import { computed, onMounted, ref } from 'vue';

interface IScript {
  id: number;
  name: string;
  content: string;
}

export const useScripts = () => {
  const PATH = '/scripts.txt';
  const idScript = ref('0');
  const scripts = ref<IScript[]>([]);
  const selectedScript = ref<IScript | null>(null);
  const content = ref('');

  const ids = computed(() => scripts.value.map(({ id }) => id));

  const onRemove = ({ id }: IScript) => {
    scripts.value = scripts.value.filter((i) => i.id !== id);
    selectedScript.value = null;
    content.value = '';
    onSave();
  };
  const onSaveScript = () => {
    if (!selectedScript.value) return;
    const { id } = selectedScript.value;
    scripts.value = scripts.value.map((i) => (i.id === id ? { ...i, content: content.value } : i));
    onSave();
  };
  const onSelect = (script: IScript) => {
    selectedScript.value = script;
    content.value = script.content;
  };

  const isScriptSave = (script: IScript) => {
    if (selectedScript.value?.id === script.id && selectedScript.value?.content !== content.value) return false;
    return true;
  };

  const addScript = (name: string) => {
    scripts.value.push({ id: +idScript.value, name: name, content: '' });
  };

  const onLoad = async () => {
    const { data } = await useFetch(`/fs?file=${PATH}`).text();
    if (!data.value) return;
    const lines = data.value.split('\n');
    scripts.value = lines.map((script): IScript => {
      const parts = script.split(':');
      const [id, name] = parts;
      return { id: +id, name, content: parts.slice(2).join(':') };
    });
  };

  const onSave = async () => {
    const text = scripts.value.map(({ id, name, content }) => `${id}:${name}:${content}`).join('\n');
    const body = new FormData();
    body.append('file[0]', new Blob([text], { type: 'text/plain' }), PATH);
    return await useFetch('/fs', { body }).post();
  };

  onMounted(() => {
    onLoad();
  });
  return {
    ids,
    content,
    scripts,
    idScript,
    selectedScript,
    onSave,
    onRemove,
    onSelect,
    addScript,
    onSaveScript,
    isScriptSave,
  };
};
