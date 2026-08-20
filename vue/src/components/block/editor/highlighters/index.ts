const escapeHtml = (value: string) => value.replaceAll('&', '&amp;').replaceAll('<', '&lt;').replaceAll('>', '&gt;');

const FULL_TOKENS =
  /(\/\/[^\n]*)|('[^'\n]*')|(\b\d+(?:\.\d+)?\b)|(\b(?:on|call|wait|while|log|if|else|end|set|get|len|ord|chr|read_temp|read_hum|read_motion)\b)|(\$[vifsp]\d+)|(\b(?:btn_\d+_\d+|btn_\d+)\b)|([;:(){}])/g;

export const fullHighlighter = (value: string) => {
  let html = '';
  let last = 0;

  for (const match of value.matchAll(FULL_TOKENS)) {
    const [fullMatch, comment, string, number, keyword, variable, event, operator] = match;

    let type = '';
    if (comment) type = 'comment';
    else if (string) type = 'string';
    else if (number) type = 'number';
    else if (keyword) type = 'keyword';
    else if (variable) type = 'variable';
    else if (event) type = 'event';
    else if (operator) type = 'operator';

    html += escapeHtml(value.slice(last, match.index));
    html += `<span class="tok--${type}">${escapeHtml(fullMatch)}</span>`;
    last = match.index + fullMatch.length;
  }

  html += escapeHtml(value.slice(last));
  return html;
};
