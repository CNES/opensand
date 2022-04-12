import {configureStore} from '@reduxjs/toolkit';
import {useDispatch as useReduxDispatch, useSelector as useReduxSelector} from 'react-redux';
import type {TypedUseSelectorHook} from 'react-redux';

import actionReducer from './action';
import errorReducer from './error';
import formReducer from './form';
import modelReducer from './model';
import pingReducer from './ping';
import projectReducer from './projects';
import sshReducer from './ssh';
import tabReducer from './tab';


const store = configureStore({
    reducer: {
        action: actionReducer,
        error: errorReducer,
        form: formReducer,
        model: modelReducer,
        ping: pingReducer,
        project: projectReducer,
        ssh: sshReducer,
        tab: tabReducer,
    },
    devTools: process.env.NODE_ENV === "development",
});


declare type Store = ReturnType<typeof store.getState>;
declare type Dispatch = typeof store.dispatch;
export interface ThunkConfig {
    state: Store;
    dispatch: Dispatch;
}


export const useDispatch = () => useReduxDispatch<Dispatch>();
export const useSelector: TypedUseSelectorHook<Store> = useReduxSelector;


export default store;
