import {createSlice, PayloadAction} from '@reduxjs/toolkit';

import {listProjectTemplates} from '../api';
import type {ITemplatesContent} from '../api';
import type {Visibility} from '../xsd/model';


interface FormState {
    visibility: Visibility;
    templates: ITemplatesContent;
    status: "idle" | "loading" | "error" | "success";
}


const initialState: FormState = {
    visibility: "NORMAL",
    templates: {},
    status: "idle",
};


const formSlice = createSlice({
    name: "form",
    initialState,
    reducers: {
        clearTemplates: (state) => {
            return {
                ...state,
                templates: {},
                status: "idle",
            };
        },
        changeVisibility: (state, action: PayloadAction<Visibility>) => {
            return {
                ...state,
                visibility: action.payload,
            };
        },
    },
    extraReducers: (builder) => {
        builder
            .addCase(listProjectTemplates.pending, (state, action) => {
                return {
                    ...state,
                    templates: {},
                    status: "loading",
                };
            })
            .addCase(listProjectTemplates.rejected, (state, action) => {
                return {
                    ...state,
                    status: "error",
                };
            })
            .addCase(listProjectTemplates.fulfilled, (state, action) => {
                return {
                    ...state,
                    templates: action.payload,
                    status: "success",
                };
            });
    },
});


export const {clearTemplates, changeVisibility} = formSlice.actions;
export default formSlice.reducer;
