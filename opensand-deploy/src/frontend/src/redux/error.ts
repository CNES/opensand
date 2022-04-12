import {createSlice, PayloadAction} from '@reduxjs/toolkit';


interface ErrorState {
    messages: string[];
    unread: number;
}


const initialState: ErrorState = {
    messages: [],
    unread: 0,
};


const errorSlice = createSlice({
    name: "error",
    initialState,
    reducers: {
        newError: (state, action: PayloadAction<string>) => {
            return {
                messages: [action.payload, ...state.messages],
                unread: state.unread + 1,
            };
        },
        removeError: (state, action: PayloadAction<number>) => {
            const index = action.payload;
            const {unread, messages} = state;
            return {
                unread,
                messages: messages.filter((m: string, i: number) => i !== index),
            };
        },
        clearErrors: (state) => {
            return {...initialState};
        },
        clearNotifications: (state) => {
            return {
                ...state,
                unread: 0,
            };
        },
    },
});


export const {newError, removeError, clearErrors, clearNotifications} = errorSlice.actions;
export default errorSlice.reducer;
