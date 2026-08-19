import { ref, onMounted, watch } from 'vue';

interface UseEditorOptions {
  initialContent?: string;
  autoFocus?: boolean;
}

export function useEditor(options: UseEditorOptions = {}) {
  const { initialContent = '<p>Начните писать...</p>', autoFocus = true } = options;

  const editorRef = ref<HTMLElement | null>(null);
  const content = ref(initialContent);
  const wordCount = ref(0);
  const charCount = ref(0);
  const selectionLength = ref(0);

  const executeCommand = (command: string, value: string | null = null) => {
    document.execCommand(command, false, value || '');
    if (editorRef.value) {
      editorRef.value.focus();
    }
    updateStats();
    updateSelectionInfo();
  };

  const updateStats = () => {
    const text = editorRef.value?.textContent || '';
    wordCount.value = text.trim() ? text.trim().split(/\s+/).length : 0;
    charCount.value = text.length;
  };

  const updateSelectionInfo = () => {
    const selection = window.getSelection();
    selectionLength.value = selection ? selection.toString().length : 0;
  };

  const handleKeyDown = (e: KeyboardEvent) => {
    if (e.ctrlKey && e.key === 'z') {
      e.preventDefault();
      executeCommand('undo');
    }
    if (e.ctrlKey && e.key === 'y') {
      e.preventDefault();
      executeCommand('redo');
    }
  };

  const getContent = (): string => {
    return editorRef.value?.innerHTML || '';
  };

  const setContent = (html: string) => {
    content.value = html;
    if (editorRef.value) {
      editorRef.value.innerHTML = html;
      updateStats();
    }
  };

  const clearContent = () => {
    setContent('');
  };

  const focus = () => {
    if (editorRef.value) {
      editorRef.value.focus();
    }
  };

  const blur = () => {
    if (editorRef.value) {
      editorRef.value.blur();
    }
  };

  const selectAll = () => {
    if (editorRef.value) {
      const range = document.createRange();
      range.selectNodeContents(editorRef.value);
      const selection = window.getSelection();
      selection?.removeAllRanges();
      selection?.addRange(range);
    }
  };

  onMounted(() => {
    if (editorRef.value) {
      content.value = initialContent;
      editorRef.value.addEventListener('keydown', handleKeyDown);

      if (autoFocus) {
        editorRef.value.focus();
      }

      updateStats();
    }
  });

  watch(content, (newContent) => {
    if (editorRef.value && editorRef.value.innerHTML !== newContent) {
      editorRef.value.innerHTML = newContent;
    }
  });

  return {
    editorRef,
    content,
    wordCount,
    charCount,
    selectionLength,

    executeCommand,
    updateStats,
    updateSelectionInfo,
    getContent,
    setContent,
    clearContent,
    focus,
    blur,
    selectAll,
  };
}
