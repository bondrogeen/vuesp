export const hexToRGBA = (hex, a = 1) => [...hex.match(/\w\w/g).map(x => parseInt(x, 16)), a];

export const int32ToBytes = (int) => ([(int >> 24) & 0xff, (int >> 16) & 0xff, (int >> 8) & 0xff, int & 0xff])

export const objCopy = (obj) => {
	try {
		return JSON.parse(JSON.stringify(obj))
	} catch (error) {
		return null
	}
}
