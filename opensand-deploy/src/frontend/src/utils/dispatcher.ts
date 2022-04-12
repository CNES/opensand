type Callback = (message: string) => void;


interface Store {
    errorCallback?: Callback;
}


const store: Store = {};


export const registerCallback = (eventName: string, callback: Callback) => {
    switch (eventName) {
        case "errorCallback":
            store.errorCallback = callback;
            return true;
        default:
            return false;
    }
};


export const unregisterCallback = (eventName: string) => {
    switch (eventName) {
        case "errorCallback":
            delete store.errorCallback;
            return true;
        default:
            return false;
    }
};


export const sendError = (error: string) => {
    const {errorCallback} = store;
    if (errorCallback != null) {
        errorCallback(error);
    }
};
