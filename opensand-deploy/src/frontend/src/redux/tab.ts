import {createSlice, PayloadAction} from '@reduxjs/toolkit';


interface TabState {
    [xsdFilename: string]: number;
}


const initialState: TabState = {
};


const tabSlice = createSlice({
    name: "tab",
    initialState,
    reducers: {
        changeTab: (state, action: PayloadAction<{xsd: string; tab: number;}>) => {
            const {xsd, tab} = action.payload;
            return {
                ...state,
                [xsd]: tab,
            };
        },
    },
});


export const {changeTab} = tabSlice.actions;
export default tabSlice.reducer;
