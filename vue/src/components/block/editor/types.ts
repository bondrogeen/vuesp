export interface IBlockEditorProps {
  value: string;
}

export interface IBlockEditorEmit {
  (e: 'update', value: string): void;
}
