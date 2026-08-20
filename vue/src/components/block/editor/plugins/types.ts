// types.ts
export interface Suggestion {
  label: string;
  type: 'keyword' | 'function' | 'variable' | 'event';
  description?: string;
  insertText?: string;
  detail?: string;
  cursorOffset?: number;
}

export interface AutocompleteOptions {
  customSuggestions?: Suggestion[];
  minPrefixLength?: number;
  maxSuggestions?: number;
  tabSize?: number;
  i18n?: (item: Suggestion) => string;
}
