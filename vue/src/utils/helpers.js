export const jsonParse = (data) => {
    try {
        return data ? JSON.parse(data) : null;
    } catch (error) {
        return data;
    }
}

const jsonString = (data) => typeof data === 'object' ? JSON.stringify(data) : data;

export const localGet = (name) => jsonParse(localStorage.getItem(name));
export const localSet = (name, data) => {
    if (localStorage) localStorage.setItem(name, jsonString(data));
};

export const sessionGet = (name) => jsonParse(sessionStorage.getItem(name));
export const sessionSet = (name, data) => {
    if (sessionStorage) sessionStorage.setItem(name, jsonString(data));
};

export const changeTheme = (value) => {
    localSet('theme', value);
    document.documentElement.classList.remove('dark')
    document.documentElement.classList.remove('light')
    document.documentElement.classList.add(value)
}
