// autocomplete.ts
import type { Plugin } from 'yace';
import { isKey } from 'yace/plugins/isKey';
import type { Suggestion, AutocompleteOptions } from './types';
import { DEFAULT_SUGGESTIONS } from './suggestions';

let suggestions: Suggestion[] = [...DEFAULT_SUGGESTIONS];
let menu: HTMLDivElement | null = null;
let selectedIndex = 0;
let currentSuggestions: Suggestion[] = [];
let timeoutId: ReturnType<typeof setTimeout> | null = null;
let currentTextarea: HTMLTextAreaElement | null = null;
let i18nFn: ((item: Suggestion) => string) | null = null;

export const autocomplete = (options: AutocompleteOptions = {}): Plugin => {
  const { customSuggestions = [], minPrefixLength = 1, maxSuggestions = 20, tabSize = 2, i18n = null } = options;

  i18nFn = i18n;
  suggestions = [...DEFAULT_SUGGESTIONS, ...customSuggestions];

  return (props, event) => {
    if (!(event instanceof KeyboardEvent)) {
      return;
    }

    const { value, selectionStart } = props;
    const textarea = event.target as HTMLTextAreaElement;
    currentTextarea = textarea;

    if (menu) {
      switch (event.key) {
        case 'ArrowDown':
          event.preventDefault();
          if (selectedIndex < currentSuggestions.length - 1) {
            selectedIndex++;
            updateMenu();
          }
          return { ...props };

        case 'ArrowUp':
          event.preventDefault();
          if (selectedIndex > 0) {
            selectedIndex--;
            updateMenu();
          }
          return { ...props };

        case 'Enter':
          event.preventDefault();
          if (currentSuggestions[selectedIndex]) {
            const result = insertSuggestion(props, currentSuggestions[selectedIndex]);
            closeMenu();
            return result;
          }
          return;

        case 'Tab':
          event.preventDefault();
          if (currentSuggestions[selectedIndex]) {
            const result = insertSuggestion(props, currentSuggestions[selectedIndex]);
            closeMenu();
            return result;
          }
          return;

        case 'Escape':
          event.preventDefault();
          closeMenu();
          return { ...props };

        case 'Backspace':
          setTimeout(() => {
            if (currentTextarea) {
              const currentValue = currentTextarea.value;
              const currentSelectionStart = currentTextarea.selectionStart;
              const start = findWordStart(currentValue, currentSelectionStart);
              const prefix = currentValue.slice(start, currentSelectionStart);

              if (prefix.length < minPrefixLength) {
                closeMenu();
              } else {
                const matches = fuzzyFilter(prefix).slice(0, maxSuggestions);
                if (matches.length === 0) {
                  closeMenu();
                } else {
                  currentSuggestions = matches;
                  selectedIndex = 0;
                  updateMenuItems(textarea, matches);
                }
              }
            }
          }, 0);
          return;
      }

      if (['ArrowDown', 'ArrowUp', 'Enter', 'Tab', 'Escape', 'Backspace'].includes(event.key)) {
        return { ...props };
      }
    }

    if (isKey('tab', event)) {
      event.preventDefault();

      const start = findWordStart(value, selectionStart);
      const prefix = value.slice(start, selectionStart);

      if (prefix.length > 0) {
        const matches = fuzzyFilter(prefix);
        if (matches.length > 0) {
          const result = insertSuggestion(props, matches[0]);
          return result;
        }
      }

      const spaces = ' '.repeat(tabSize);
      const newValue = value.slice(0, selectionStart) + spaces + value.slice(selectionStart);
      const newPosition = selectionStart + tabSize;

      return {
        value: newValue,
        selectionStart: newPosition,
        selectionEnd: newPosition,
      };
    }

    if (event.key.length !== 1 || event.ctrlKey || event.metaKey || event.altKey) {
      return;
    }

    if (timeoutId) {
      clearTimeout(timeoutId);
    }

    timeoutId = setTimeout(() => {
      timeoutId = null;

      const currentValue = textarea.value;
      const currentSelectionStart = textarea.selectionStart;
      const start = findWordStart(currentValue, currentSelectionStart);
      const prefix = currentValue.slice(start, currentSelectionStart);

      if (prefix.length < minPrefixLength) {
        closeMenu();
        return;
      }

      let matches = fuzzyFilter(prefix);
      matches = matches.slice(0, maxSuggestions);

      if (matches.length === 0) {
        closeMenu();
        return;
      }

      currentSuggestions = matches;
      selectedIndex = 0;

      if (menu) {
        updateMenuItems(textarea, matches);
      } else {
        showMenu(textarea, matches);
      }
    }, 0);

    return { ...props };
  };
};

function fuzzyFilter(prefix: string): Suggestion[] {
  const search = prefix.toLowerCase();

  return suggestions
    .filter((s) => {
      const label = s.label.toLowerCase();
      if (label.startsWith(search)) return true;
      if (label.includes(search)) return true;
      const firstLetters = label
        .split('_')
        .map((w) => w[0])
        .join('');
      if (firstLetters.startsWith(search)) return true;
      return false;
    })
    .sort((a, b) => {
      const aLabel = a.label.toLowerCase();
      const bLabel = b.label.toLowerCase();

      const aStarts = aLabel.startsWith(search);
      const bStarts = bLabel.startsWith(search);
      if (aStarts && !bStarts) return -1;
      if (!aStarts && bStarts) return 1;

      const aIndex = aLabel.indexOf(search);
      const bIndex = bLabel.indexOf(search);
      if (aIndex !== bIndex) return aIndex - bIndex;

      if (aLabel.length !== bLabel.length) {
        return aLabel.length - bLabel.length;
      }

      return aLabel.localeCompare(bLabel);
    });
}

function findWordStart(value: string, position: number): number {
  let start = position;
  while (start > 0 && /[a-zA-Z0-9_$]/.test(value[start - 1])) {
    start--;
  }
  return start;
}

function translateItem(item: Suggestion): { label: string; detail: string; description: string } {
  const detail = i18nFn ? i18nFn(item) : item.detail || '';

  return {
    label: item.label,
    detail: detail,
    description: item.description || '',
  };
}

function showMenu(textarea: HTMLTextAreaElement, suggestions: Suggestion[]) {
  closeMenu();

  menu = document.createElement('div');
  menu.className = 'yace-autocomplete';

  const coords = getCaretCoordinates(textarea);

  Object.assign(menu.style, {
    position: 'fixed',
    left: `${coords.left}px`,
    top: `${coords.top + 24}px`,
    background: '#1e1e1e',
    border: '1px solid #404040',
    borderRadius: '6px',
    maxHeight: '250px',
    overflowY: 'auto',
    minWidth: '250px',
    maxWidth: '400px',
    zIndex: '10000',
    fontFamily: "'Consolas', 'Monaco', monospace",
    fontSize: '14px',
    boxShadow: '0 8px 24px rgba(0,0,0,0.5)',
    padding: '4px 0',
  });

  suggestions.forEach((suggestion, index) => {
    const item = createMenuItem(suggestion, index);
    menu!.appendChild(item);
  });

  document.body.appendChild(menu);
  updateMenu();
}

function updateMenuItems(textarea: HTMLTextAreaElement, suggestions: Suggestion[]) {
  if (!menu) {
    showMenu(textarea, suggestions);
    return;
  }

  menu.innerHTML = '';

  suggestions.forEach((suggestion, index) => {
    const item = createMenuItem(suggestion, index);
    menu!.appendChild(item);
  });

  updateMenu();
}

function createMenuItem(suggestion: Suggestion, index: number): HTMLDivElement {
  const item = document.createElement('div');
  item.className = 'yace-autocomplete-item';

  const colors: Record<string, string> = {
    keyword: '#569cd6',
    function: '#dcdcaa',
    variable: '#9cdcfe',
    event: '#c586c0',
  };

  const translated = translateItem(suggestion);

  item.innerHTML = `
    <div style="display: flex; align-items: center; gap: 8px; flex: 1;">
      <span style="color: ${colors[suggestion.type] || '#d4d4d4'};">
        ${translated.label}
      </span>
      ${translated.detail ? `<span style="color: #808080; font-size: 12px; margin-left: 8px;">${translated.detail}</span>` : ''}
    </div>
    ${translated.description ? `<span style="color: #808080; font-size: 12px;">${translated.description}</span>` : ''}
  `;

  Object.assign(item.style, {
    padding: '6px 12px',
    cursor: 'pointer',
    display: 'flex',
    justifyContent: 'space-between',
    alignItems: 'center',
    borderLeft: '2px solid transparent',
    transition: 'background 0.1s',
  });

  if (index === selectedIndex) {
    item.style.background = '#094771';
    item.style.borderLeftColor = '#0078d4';
  }

  item.addEventListener('mouseenter', () => {
    selectedIndex = index;
    updateMenu();
  });

  item.addEventListener('mousedown', (e) => {
    e.preventDefault();
    e.stopPropagation();
    if (currentTextarea) {
      const props = {
        value: currentTextarea.value,
        selectionStart: currentTextarea.selectionStart,
        selectionEnd: currentTextarea.selectionEnd,
      };
      const result = insertSuggestion(props, suggestion);
      currentTextarea.value = result.value;
      currentTextarea.selectionStart = result.selectionStart;
      currentTextarea.selectionEnd = result.selectionEnd;
      currentTextarea.dispatchEvent(new Event('input', { bubbles: true }));
      closeMenu();
      currentTextarea.focus();
    }
  });

  return item;
}

function updateMenu() {
  if (!menu) return;

  const items = menu.querySelectorAll('.yace-autocomplete-item');
  items.forEach((item, index) => {
    const div = item as HTMLDivElement;
    if (index === selectedIndex) {
      div.style.background = '#094771';
      div.style.borderLeftColor = '#0078d4';
      div.scrollIntoView({ block: 'nearest' });
    } else {
      div.style.background = 'transparent';
      div.style.borderLeftColor = 'transparent';
    }
  });
}

function insertSuggestion(props: { value: string; selectionStart: number; selectionEnd: number }, suggestion: Suggestion) {
  const { value, selectionStart } = props;
  const start = findWordStart(value, selectionStart);

  const insertText = suggestion.insertText || suggestion.label;

  const placeholderRegex = /\$\{([^}]*)\}/g;
  let cleanText = insertText;
  const placeholders: { index: number; text: string; length: number }[] = [];
  let match;

  while ((match = placeholderRegex.exec(insertText)) !== null) {
    const text = match[1];
    const index = match.index;
    const length = match[0].length;
    placeholders.push({ index, text, length });
  }

  cleanText = insertText.replace(/\$\{([^}]*)\}/g, '$1');

  const firstPlaceholder = placeholders[0];

  let cursorStart = start + cleanText.length;
  let cursorEnd = start + cleanText.length;

  if (firstPlaceholder) {
    let cleanIndex = 0;
    let insertIndex = 0;
    let found = false;

    while (insertIndex < insertText.length && !found) {
      const placeholder = placeholders.find((p) => p.index === insertIndex);
      if (placeholder) {
        cleanIndex += placeholder.text.length;
        insertIndex += placeholder.length;
        if (placeholder === firstPlaceholder) {
          cursorStart = start + cleanIndex - placeholder.text.length;
          cursorEnd = start + cleanIndex;
          found = true;
        }
      } else {
        cleanIndex++;
        insertIndex++;
      }
    }
  }

  const newValue = value.slice(0, start) + cleanText + value.slice(selectionStart);

  return {
    value: newValue,
    selectionStart: cursorStart,
    selectionEnd: cursorEnd,
  };
}

function closeMenu() {
  if (timeoutId) {
    clearTimeout(timeoutId);
    timeoutId = null;
  }

  if (menu) {
    menu.remove();
    menu = null;
    currentSuggestions = [];
    selectedIndex = 0;
    currentTextarea = null;
  }
}

function getCaretCoordinates(textarea: HTMLTextAreaElement) {
  const rect = textarea.getBoundingClientRect();
  const { value, selectionStart } = textarea;
  const textBefore = value.substring(0, selectionStart);
  const lines = textBefore.split('\n');
  const currentLine = lines.length;
  const lastLine = lines[lines.length - 1] || '';

  const lineHeight = parseInt(getComputedStyle(textarea).lineHeight) || 20;
  const charWidth = 8;

  return {
    left: rect.left + 10 + lastLine.length * charWidth,
    top: rect.top + (currentLine - 1) * lineHeight,
  };
}

document.addEventListener('mousedown', (e) => {
  const target = e.target as Node;
  if (menu && !menu.contains(target)) {
    closeMenu();
  }
});
