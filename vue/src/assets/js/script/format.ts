export const removeLBScript = (s: string) => s.replace(/(\r\n|\n|\r)/g, '');

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
  let lines = s.split('\n');
  let indentLevel = 0;
  const spaces = '    ';

  for (let i = 0; i < lines.length; i++) {
    let line = lines[i];

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
