import type { Plugin } from 'yace';

export const autoSemicolon = (): Plugin => {
  return (props, event) => {
    if (!(event instanceof KeyboardEvent) || event.key !== 'Enter') {
      return;
    }

    const { value, selectionStart, selectionEnd } = props;

    if (selectionStart !== selectionEnd) {
      return;
    }

    const lineStart = value.lastIndexOf('\n', selectionStart - 1) + 1;
    const lineEnd = value.indexOf('\n', selectionStart);
    const currentLineEnd = lineEnd === -1 ? value.length : lineEnd;

    const textAfterCursor = value.slice(selectionStart, currentLineEnd);
    if (textAfterCursor.trim().length > 0) {
      return;
    }

    const currentLine = value.substring(lineStart, currentLineEnd);
    const trimmedLine = currentLine.trimEnd();

    if (trimmedLine.endsWith(';') || trimmedLine.length === 0) {
      return;
    }

    const newValue = value.slice(0, currentLineEnd) + ';' + value.slice(currentLineEnd);
    const newPosition = selectionStart + 1;

    return {
      value: newValue,
      selectionStart: newPosition,
      selectionEnd: newPosition,
    };
  };
};
