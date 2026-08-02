export const removeLBScript = (code: string) => {
  let result = '';
  let inString = false;
  let quote = '';
  for (let i = 0; i < code.length; i++) {
    const ch = code[i];
    if ((ch === '"' || ch === "'") && (i === 0 || code[i - 1] !== '\\')) {
      if (!inString) {
        inString = true;
        quote = ch;
        result += ch;
      } else if (ch === quote) {
        inString = false;
        result += ch;
      } else {
        result += ch;
      }
      continue;
    }
    if (inString) {
      result += ch;
      continue;
    }
    if (ch === ' ' || ch === '\t' || ch === '\n' || ch === '\r') {
      continue;
    }
    result += ch;
  }
  return result;
};

export const formatScript = (s: string) => {
  let result = s;
  result = lineBreak(result);
  result = addIndent(result);
  return result;
};

function lineBreak(s: string) {
  let r = '';
  for (let i = 0; i < s.length; i++) {
    r += s[i];
    if (s[i] === ';' || s[i].match(/(if|while):/)) r += '\n';
  }
  return r;
}

function addIndent(s: string) {
  let r = '';
  const lines = s.split('\n');
  let indentLevel = 0;
  const spaces = '    ';

  for (let i = 0; i < lines.length; i++) {
    const line = lines[i];

    if (line.match(/^(if|while|else|on).*;/)) {
      if (line.match(/^else;/)) {
        indentLevel--;
        if (indentLevel < 0) indentLevel = 0;
        r += spaces.repeat(indentLevel) + line + '\n';
        indentLevel++;
      } else {
        r += spaces.repeat(indentLevel) + line + '\n';
        indentLevel++;
      }
    } else if (line === 'end;') {
      indentLevel--;
      if (indentLevel < 0) indentLevel = 0;
      r += spaces.repeat(indentLevel) + line + '\n';
    } else {
      r += spaces.repeat(indentLevel) + line + '\n';
    }
  }

  return r;
}
