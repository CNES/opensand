import {createSlice} from '@reduxjs/toolkit';

import {createProject, copyProject, deleteProject, listProjects, uploadProject} from '../api';


interface ProjectState {
    projects: string[];
    status: "idle" | "loading" | "error" | "created" | "success";
}


const initialState: ProjectState = {
    projects: [],
    status: "idle",
};


const projectSlice = createSlice({
    name: "projects",
    initialState,
    reducers: {
        clearProjects: (state) => {
            return {
                projects: [],
                status: "idle",
            };
        },
    },
    extraReducers: (builder) => {
        builder
            .addCase(listProjects.pending, (state, action) => {
                return {
                    ...state,
                    status: "loading",
                };
            })
            .addCase(listProjects.rejected, (state, action) => {
                return {
                    ...state,
                    status: "error",
                };
            })
            .addCase(listProjects.fulfilled, (state, action) => {
                const {projects} = action.payload;
                return {
                    projects,
                    status: "success",
                };
            })
            .addCase(createProject.rejected, (state, action) => {
                return {
                    ...state,
                    status: "error",
                };
            })
            .addCase(createProject.pending, (state, action) => {
                return {
                    ...state,
                    status: "loading",
                };
            })
            .addCase(createProject.fulfilled, (state, action) => {
                const {project} = action.meta.arg;
                return {
                    projects: [project, ...state.projects],
                    status: "created",
                };
            })
            .addCase(copyProject.rejected, (state, action) => {
                return {
                    ...state,
                    status: "error",
                };
            })
            .addCase(copyProject.pending, (state, action) => {
                return {
                    ...state,
                    status: "loading",
                };
            })
            .addCase(copyProject.fulfilled, (state, action) => {
                const {name} = action.meta.arg;
                return {
                    projects: [name, ...state.projects],
                    status: "created",
                };
            })
            .addCase(uploadProject.rejected, (state, action) => {
                return {
                    ...state,
                    status: "error",
                };
            })
            .addCase(uploadProject.pending, (state, action) => {
                return {
                    ...state,
                    status: "loading",
                };
            })
            .addCase(uploadProject.fulfilled, (state, action) => {
                const {project} = action.meta.arg;
                return {
                    projects: [project, ...state.projects],
                    status: "created",
                };
            })
            .addCase(deleteProject.rejected, (state, action) => {
                return {
                    ...state,
                    status: "error",
                };
            })
            .addCase(deleteProject.pending, (state, action) => {
                return {
                    ...state,
                    status: "loading",
                };
            })
            .addCase(deleteProject.fulfilled, (state, action) => {
                const {project} = action.meta.arg;
                return {
                    projects: state.projects.filter((name: string) => name !== project),
                    status: "idle",
                };
            });
    },
});


export const {clearProjects} = projectSlice.actions;
export default projectSlice.reducer;
