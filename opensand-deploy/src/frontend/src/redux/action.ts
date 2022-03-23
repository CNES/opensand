import {createSlice, PayloadAction} from '@reduxjs/toolkit';

import {copyEntityConfiguration, deployEntity} from '../api';


interface ActionState {
    [entity: string]: {
        running: boolean;
        status: "idle" | "pending" | "error" | "success";
    };
}


const initialState: ActionState = {
};


const actionSlice = createSlice({
    name: "action",
    initialState,
    reducers: {
        clearActions: (state) => {
            return {};
        },
        createEntityAction: (state, action: PayloadAction<string>) => {
            const entity = action.payload;
            return {
                ...state,
                [entity]: state[entity] || {running: false, status: "idle"},
            };
        },
    },
    extraReducers: (builder) => {
        builder
            .addCase(deployEntity.pending, (state, action) => {
                const {entity} = action.meta.arg;
                const running = state[entity]?.running || false;
                return {
                    ...state,
                    [entity]: {running, status: "pending"},
                };
            })
            .addCase(deployEntity.rejected, (state, action) => {
                const {entity} = action.meta.arg;
                const running = state[entity]?.running || false;
                return {
                    ...state,
                    [entity]: {running, status: "error"},
                };
            })
            .addCase(deployEntity.fulfilled, (state, action) => {
                const {entity} = action.meta.arg;
                const running = action.payload.running || false;
                return {
                    ...state,
                    [entity]: {running, status: "success"},
                };
            })
            .addCase(copyEntityConfiguration.pending, (state, action) => {
                const {entity} = action.meta.arg;
                const running = state[entity]?.running || false;
                return {
                    ...state,
                    [entity]: {running, status: "pending"},
                };
            })
            .addCase(copyEntityConfiguration.rejected, (state, action) => {
                const {entity} = action.meta.arg;
                const running = state[entity]?.running || false;
                return {
                    ...state,
                    [entity]: {running, status: "error"},
                };
            })
            .addCase(copyEntityConfiguration.fulfilled, (state, action) => {
                const {entity} = action.meta.arg;
                const running = state[entity]?.running || false;
                return {
                    ...state,
                    [entity]: {running, status: "success"},
                };
            });
    },
});


export const {clearActions, createEntityAction} = actionSlice.actions;
export default actionSlice.reducer;
