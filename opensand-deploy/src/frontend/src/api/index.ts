import {createAsyncThunk} from '@reduxjs/toolkit'
import type {AsyncThunkPayloadCreator, AsyncThunkOptions} from '@reduxjs/toolkit';

import type {ThunkConfig} from '../redux/';
import {newError} from '../redux/error';

import {toXML, fromXML, fromXSD, isComponentElement, isParameterElement} from '../xsd';
import type {Model, Component} from '../xsd';


export const asyncThunk = <Returned, ThunkArg = void, ExtraThunkConfig = {}>(
    typePrefix: string,
    payloadCreator: AsyncThunkPayloadCreator<Returned, ThunkArg, ThunkConfig & ExtraThunkConfig>,
    options?: AsyncThunkOptions<ThunkArg, ThunkConfig & ExtraThunkConfig>,
) => createAsyncThunk(typePrefix, payloadCreator, options);


declare type Dispatch = Pick<ThunkConfig, "dispatch">["dispatch"];


export interface IApiSuccess {
    status: string;
}


export interface IPidSuccess extends IApiSuccess {
    running?: boolean;
};


export interface IPingSuccess extends IApiSuccess {
    ping: string;
};


export interface IPingDestinations extends IApiSuccess {
    addresses: string[];
};


export interface IFileContent extends IApiSuccess {
    content: string;
}


export interface IProjectsContent extends IApiSuccess {
    projects: string[];
}


export interface ITemplatesContent {
    [name: string]: string[];
}


const apiCall = async (route: string, configuration: RequestInit, dispatch: Dispatch): Promise<Response> => {
    try {
        return await fetch(/*encodeURI(route)*/ route, configuration);
    } catch (err) {
        dispatch(newError("HTTP request error: " + err));
        throw err;
    }
};


const doApiCall = async <T>(url: string, configuration: RequestInit, dispatch: Dispatch): Promise<T> => {
    const response = await apiCall(url, configuration, dispatch);

    let error = null;
    try {
        const payload = await response.json();
        if ('error' in payload) {
            const msg = "Error when processing the request: " + payload.error;
            dispatch(newError(msg));
            error = new Error(msg);
        } else {
            return payload;
        }
    } catch (err) {
        if (response.ok) {
            dispatch(newError("Error in the HTTP response: " + err));
        } else {
            dispatch(newError("HTTP request sent back error code: " + response.status + " " + response.statusText));
        }
        error = err;
    }
    throw error;
};


const doFetch = async <T>(
        url: string,
        dispatch: Dispatch,
        method: string = "GET",
        body?: object,
): Promise<T> => {
    const configuration: RequestInit = {method, credentials: "same-origin"};
    if (body) {
        configuration.body = JSON.stringify(body);
        configuration.headers = {
            "Accept": "application/json",
            "Content-Type": "application/json",
        };
    }

    return await doApiCall<T>(url, configuration, dispatch);
};


const doGetXSD = async (dispatch: Dispatch, filename?: string): Promise<Model> => {
    const url = filename == null ? "/api/project" : `/api/model/${filename}`;
    const content = await doFetch<IFileContent>(url, dispatch);
    try {
        return fromXSD(content.content);
    } catch (err) {
        dispatch(newError("Cannot parse XSD: " + err));
        throw err;
    }
};


const doGetXML = async (dispatch: Dispatch, project: string, xsd?: string, urlFragment?: string): Promise<Model> => {
    const dataModel = await doGetXSD(dispatch, xsd);

    const url = "/api/project/" + project + (urlFragment == null ? "" : "/" + urlFragment);
    const xml = await doFetch<IFileContent>(url, dispatch);
    try {
        return fromXML(dataModel, xml.content);
    } catch (err) {
        dispatch(newError("Cannot parse XML: " + err));
        throw err;
    }
};


const serializeXML = (model: Model, dispatch: Dispatch) => {
    try {
        return toXML(model);
    } catch (err) {
        dispatch(newError("Cannot serialize XML: " + err));
        throw err;
    }
};


export const getXML = asyncThunk<Model, {project: string; xsd: string; urlFragment: string;}>(
    'model/getXML',
    async ({project, xsd, urlFragment}, {dispatch}) => {
        return await doGetXML(dispatch, project, xsd, urlFragment);
    },
);


export const forceEntityInXML = asyncThunk<IApiSuccess, {project: string; xsd: string; loadUrl: string; saveUrl: string; entity: string;}>(
    'model/forceEntityInXML',
    async ({project, xsd, loadUrl, saveUrl, entity}, {dispatch}) => {
        const model = await doGetXML(dispatch, project, xsd, loadUrl);
        const entityComponent = model.root.elements.find((e) => e.element.id === "entity");
        if (isComponentElement(entityComponent)) {
            const entityParameter = entityComponent.element.elements.find((e) => e.element.id === "entity_type");
            if (isParameterElement(entityParameter)) {
                entityParameter.element.value = entity;
                const xml_data = serializeXML(model, dispatch);
                return await doFetch<IApiSuccess>(`/api/project/${project}/${saveUrl}`, dispatch, "PUT", {xml_data});
            }
        }
        return {status: "Not changed"};
    },
);


export const updateXML = asyncThunk<IApiSuccess, {project: string; xsd: string; urlFragment: string; root?: Component}>(
    'model/updateXML',
    async ({project, xsd, urlFragment, root}, {getState, dispatch}) => {
        const {model} = getState().model;
        if (!model) {
            const msg = "Cannot update model for project " + project + ": model is not loaded.";
            dispatch(newError(msg));
            throw new Error(msg);
        }

        const xml_data = serializeXML(root ? {...model, root} : model, dispatch);
        const response = await doFetch<IApiSuccess>(`/api/project/${project}/${urlFragment}`, dispatch, "PUT", {xml_data});
        dispatch(getXML({project, xsd, urlFragment}));
        return response;
    },
);


export const deleteXML = asyncThunk<IApiSuccess, {project: string; urlFragment: string;}>(
    'model/deleteXML',
    async ({project, urlFragment}, {dispatch}) => {
        return await doFetch<IApiSuccess>(`/api/project/${project}/${urlFragment}`, dispatch, "DELETE");
    },
);


export const getProject = asyncThunk<Model, {project: string;}>(
    'project/getProject',
    async ({project}, {dispatch}) => {
        return await doGetXML(dispatch, project);
    },
);


export const updateProject = asyncThunk<IApiSuccess, {project: string; root: Component;}>(
    'project/updateProject',
    async ({project, root}, {getState, dispatch}) => {
        const {model} = getState().model;
        if (!model) {
            const msg = "Cannot update project " + project + ": model is not loaded.";
            dispatch(newError(msg));
            throw new Error(msg);
        }

        const xml_data = serializeXML({...model, root}, dispatch);
        const response = await doFetch<IApiSuccess>("/api/project/" + project, dispatch, "PUT", {xml_data});
        dispatch(getProject({project}));
        return response;
    },
);


export const listProjectTemplates = asyncThunk<ITemplatesContent, {project: string;}>(
    'project/listProjectTemplates',
    async ({project}, {dispatch}) => {
        return await doFetch<ITemplatesContent>("/api/project/" + project + "/templates", dispatch);
    },
);


export const listProjects = asyncThunk<IProjectsContent>(
    'projects/listProjects',
    async (_, {dispatch}) => {
        return await doFetch<IProjectsContent>("/api/projects", dispatch);
    },
);


export const createProject = asyncThunk<IApiSuccess, {project: string;}>(
    'projects/createProject',
    async ({project}, {dispatch}) => {
        const dataModel = await doGetXSD(dispatch);
        dataModel.root.elements.forEach((e) => {
            if (e.type === "component" && e.element.id === "platform") {
                e.element.elements.forEach((p) => {
                    if (p.type === "parameter" && p.element.id === "project") {
                        p.element.value = project;
                    }
                });
            }
        });

        const xml_data = serializeXML(dataModel, dispatch);
        return await doFetch<IApiSuccess>("/api/project/" + project, dispatch, "PUT", {xml_data});
    },
);


export const copyProject = asyncThunk<IApiSuccess, {project: string; name: string;}>(
    'projects/copyProject',
    async ({project, name}, {dispatch}) => {
        return await doFetch<IApiSuccess>("/api/project/" + project, dispatch, "POST", {name});
    },
);


export const uploadProject = asyncThunk<IApiSuccess, {project: string; archive: File;}>(
    'projects/uploadProject',
    async ({project, archive}, {dispatch}) => {
        const form = new FormData ();
        form.append("project", archive);

        const configuration: RequestInit = {
            method: "POST",
            body: form,
            credentials: "same-origin",
            headers: {Accept: "application/json"},
        };

        return await doApiCall<IApiSuccess>("/api/project/" + project, configuration, dispatch);
    },
);


export const deleteProject = asyncThunk<IApiSuccess, {project: string;}>(
    'projects/deleteProject',
    async ({project}, {dispatch}) => {
        return await doFetch<IApiSuccess>("/api/project/" + project, dispatch, "DELETE");
    },
);


export const copyEntityConfiguration = asyncThunk<IApiSuccess, {project: string; entity: string; folder: string;}>(
    'action/copyEntityConfiguration',
    async ({project, entity, folder}, {dispatch}) => {
        const body = {destination_folder: folder, copy_method: "NFS"};
        return await doFetch<IApiSuccess>(`/api/project/${project}/entity/${entity}`, dispatch, "PUT", body);
    },
);


interface SshParameters {
    project: string;
    entity: string;
    address: string;
}


export interface DeployParameters extends SshParameters {
    folder: string;
    copyMethod: string;
    runMethod: string;
}


export const deployEntity = asyncThunk<IPidSuccess, DeployParameters>(
    'action/deployEntity',
    async ({project, entity, folder, copyMethod, runMethod, address}, {getState, dispatch}) => {
        const sshConfig = getState().ssh;
        if (!sshConfig?.configured) {
            const msg = "Cannot deploy entity " + entity + " of project " + project + ": SSH Credentials not set";
            dispatch(newError(msg));
            throw new Error(msg);
        }

        const {password, isPassphrase} = sshConfig;
        const body = {
            destination_folder: folder,
            copy_method: copyMethod,
            run_method: runMethod,
            ssh: {address, password, is_passphrase: isPassphrase},
        };
        const response = await doFetch<IPidSuccess>(`/api/project/${project}/entity/${entity}`, dispatch, "PUT", body);

        if (response.running) {
            setTimeout(() => dispatch(deployEntity({
                project, entity, address,
                folder: "", copyMethod: "",
                runMethod: "STATUS",
            })), runMethod === "STATUS" ? 5000 : 500);
        }
        return response;
    },
);


interface PingParameters extends SshParameters {
    destination: string;
}


export const pingEntity = asyncThunk<IPingSuccess, PingParameters>(
    'action/pingEntity',
    async ({project, entity, destination, address}, {getState, dispatch}) => {
        const sshConfig = getState().ssh;
        if (!sshConfig?.configured) {
            const msg = "Cannot ping from entity " + entity + " of project " + project + ": SSH Credentials not set";
            dispatch(newError(msg));
            throw new Error(msg);
        }

        const {password, isPassphrase} = sshConfig;
        const body = {
            run_method: "PING",
            ping_address: destination,
            ssh: {address, password, is_passphrase: isPassphrase},
        };
        return await doFetch<IPingSuccess>(`/api/project/${project}/entity/${entity}`, dispatch, "PUT", body);
    },
);


export const findPingDestinations = asyncThunk<IPingDestinations, {project: string;}>(
    'action/findPingDestinations',
    async ({project}, {dispatch}) => {
        return await doFetch<IPingDestinations>(`/api/project/${project}/ping`, dispatch);
    },
);
