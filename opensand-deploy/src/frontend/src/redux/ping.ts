import {createSlice, PayloadAction} from '@reduxjs/toolkit';

import {pingEntity, findPingDestinations} from '../api';


interface PingState {
    destinations: string[];
    source?: {name: string; address: string;};
    result?: string;
    status: "idle" | "pending" | "loading" | "selection" | "error" | "success";
}


const initialState: PingState = {
    destinations: [],
    status: "idle",
};


const pingSlice = createSlice({
    name: "ping",
    initialState,
    reducers: {
        clearPing: (state) => {
            return {...initialState};
        },
        closePing: (state) => {
            return {
                ...state,
                status: "idle",
            };
        },
        setPingingEntity: (state, action: PayloadAction<{name: string; address: string;} | undefined>) => {
            return {
                ...initialState,
                source: action.payload,
            };
        },
    },
    extraReducers: (builder) => {
        builder
            .addCase(findPingDestinations.pending, (state, action) => {
                state.status = "loading";
            })
            .addCase(findPingDestinations.rejected, (state, action) => {
                state.status = "error";
            })
            .addCase(findPingDestinations.fulfilled, (state, action) => {
                state.status = "selection";
                state.destinations = action.payload.addresses;
            })
            .addCase(pingEntity.pending, (state, action) => {
                state.status = "pending";
            })
            .addCase(pingEntity.rejected, (state, action) => {
                state.status = "error";
            })
            .addCase(pingEntity.fulfilled, (state, action) => {
                state.status = "success";
                state.result = action.payload.ping;
            });
    },
});


export const {clearPing, closePing, setPingingEntity} = pingSlice.actions;
export default pingSlice.reducer;
