import {createSlice} from '@reduxjs/toolkit';

import {getProject, forceEntityInXML, updateProject, getXML, updateXML} from '../api';
import type {Model} from '../xsd';


interface ModelState {
    model?: Model;
    status: "idle" | "pending" | "error" | "saved" | "success";
}


const initialState: ModelState = {
    status: "idle",
};


const modelSlice = createSlice({
    name: "model",
    initialState,
    reducers: {
        clearModel: (state) => {
            return { status: "idle" };
        },
    },
    extraReducers: (builder) => {
        builder
            .addCase(getProject.rejected, (state, action) => {
                state.status = "error";
            })
            .addCase(getProject.pending, (state, action) => {
                state.status = "pending";
            })
            .addCase(getProject.fulfilled, (state, action) => {
                return {
                    model: action.payload,
                    status: "success",
                };
            })
            .addCase(updateProject.rejected, (state, action) => {
                state.status = "error";
            })
            .addCase(updateProject.pending, (state, action) => {
                state.status = "pending";
            })
            .addCase(updateProject.fulfilled, (state, action) => {
                state.status = "saved";
            })
            .addCase(getXML.rejected, (state, action) => {
                state.status = "error";
            })
            .addCase(getXML.pending, (state, action) => {
                state.status = "pending";
            })
            .addCase(getXML.fulfilled, (state, action) => {
                return {
                    model: action.payload,
                    status: "success",
                };
            })
            .addCase(updateXML.rejected, (state, action) => {
                state.status = "error";
            })
            .addCase(updateXML.pending, (state, action) => {
                state.status = "pending";
            })
            .addCase(updateXML.fulfilled, (state, action) => {
                state.status = "saved";
            })
            .addCase(forceEntityInXML.pending, (state, action) => {
                state.status = "pending";
            })
            .addCase(forceEntityInXML.fulfilled, (state, action) => {
                state.status = "success";
            });
    },
});


export const {clearModel} = modelSlice.actions;
export default modelSlice.reducer;
