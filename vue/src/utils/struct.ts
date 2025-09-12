import VuespStruct from 'vuesp-struct';

const struct = new VuespStruct();

const res = await (await fetch(`/struct.json`, { method: 'GET' })).json();

struct.init(res);

export { struct };
