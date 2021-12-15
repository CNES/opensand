import {Model} from '../xsd/model';
import {toXML} from '../xsd/serialiser';


type TCallback<T> = (x: T) => void;
type ErrorCallback = TCallback<string>;


interface IApiError {
    error: string;
}


export interface IApiSuccess {
    status: string;
}


export const silenceSuccess = (success: IApiSuccess) => {
};


export interface IXsdContent {
    content: string;
}


export interface IXmlContent {
    content: string;
}


export interface IProjectsContent {
    projects: string[];
}


export interface ITemplatesContent {
    [name: string]: string[];
}


const onApiResponse = <T>(
        onSuccess: TCallback<T>,
        onError: ErrorCallback,
): TCallback<IApiError | T> => {
    const callback = (result: IApiError | T) => {
        if ('error' in result) {
            onError("Error when processing the request: " + result.error);
        } else {
            onSuccess(result);
        }
    };
    return callback;
};

const doFetch = <T>(
        callback: TCallback<T>,
        errorCallback: ErrorCallback,
        url: string,
        method: string = "GET",
        body?: object
): Promise<void> => {
    const configuration: RequestInit = {method, credentials: "same-origin"};
    if (body) {
        configuration.body = JSON.stringify(body);
        configuration.headers = {
            "Accept": "application/json",
            "Content-Type": "application/json",
        };
    }

    return fetch(url, configuration).then(
        (response: Response) => response.json().then(onApiResponse<T>(callback, errorCallback)).catch(
            (ex) => {
                if (response.ok) {
                    errorCallback("Error in the HTTP response: " + ex);
                } else {
                    errorCallback("HTTP request sent back error code: " + response.status + " " + response.statusText);
                }
            }
        )
    ).catch(
        (ex) => errorCallback("HTTP request error: " + ex)
    );
};


export const getXSD = (
        callback: TCallback<IXsdContent>,
        errorCallback: ErrorCallback,
        filename: string,
): Promise<void> => doFetch<IXsdContent>(callback, errorCallback, `/api/model/${filename}`);


export const listProjects = (
        callback: TCallback<IProjectsContent>,
        errorCallback: ErrorCallback,
): Promise<void> => doFetch<IProjectsContent>(callback, errorCallback, "/api/projects");


export const getProjectModel = (
        callback: TCallback<IXsdContent>,
        errorCallback: ErrorCallback,
): Promise<void> => doFetch<IXsdContent>(callback, errorCallback, "/api/project");


export const listProjectTemplates = (
        callback: TCallback<ITemplatesContent>,
        errorCallback: ErrorCallback,
        projectName: string,
): Promise<void> => doFetch<ITemplatesContent>(callback, errorCallback, "/api/project/" + projectName + "/templates");


export const getProject = (
        callback: TCallback<IXmlContent>,
        errorCallback: ErrorCallback,
        projectName: string,
): Promise<void> => doFetch<IXmlContent>(callback, errorCallback, "/api/project/" + projectName);


export const copyProject = (
        callback: TCallback<IApiSuccess>,
        errorCallback: ErrorCallback,
        projectName: string,
        newProjectName: string,
): Promise<void> => doFetch<IApiSuccess>(callback, errorCallback, "/api/project/" + projectName, "POST", {name: newProjectName});


export const updateProject = (
        callback: TCallback<IApiSuccess>,
        errorCallback: ErrorCallback,
        projectName: string,
        project: Model,
): Promise<void> => doFetch<IApiSuccess>(callback, errorCallback, "/api/project/" + projectName, "PUT", {xml_data: toXML(project)});


export const uploadProject = (
        callback: TCallback<IApiSuccess>,
        errorCallback: ErrorCallback,
        projectName: string,
        projectArchive: File,
): Promise<void> => {
    const form = new FormData ();
    form.append("project", projectArchive);

    const configuration: RequestInit = {
        method: "POST",
        body: form,
        credentials: "same-origin",
        headers: {Accept: "application/json"},
    };

    return fetch("/api/project/" + projectName, configuration).then(
        (response: Response) => response.json().then(onApiResponse<IApiSuccess>(callback, errorCallback)).catch(
            (ex) => {
                if (response.ok) {
                    errorCallback("Error in the HTTP response: " + ex);
                } else {
                    errorCallback("HTTP request sent back error code: " + response.status + " " + response.statusText);
                }
            }
        )
    ).catch(
        (ex) => errorCallback("HTTP request error: " + ex)
    );
};


export const deleteProject = (
        callback: TCallback<IApiSuccess>,
        errorCallback: ErrorCallback,
        projectName: string,
): Promise<void> => doFetch<IApiSuccess>(callback, errorCallback, "/api/project/" + projectName, "DELETE");


export const getProjectXML = (
        callback: TCallback<IXmlContent>,
        errorCallback: ErrorCallback,
        projectName: string,
        urlFragment: string,
): Promise<void> => doFetch<IXmlContent>(callback, errorCallback, `/api/project/${projectName}/${urlFragment}`);


export const updateProjectXML = (
        callback: TCallback<IApiSuccess>,
        errorCallback: ErrorCallback,
        projectName: string,
        urlFragment: string,
        model: Model,
): Promise<void> => doFetch<IApiSuccess>(callback, errorCallback, `/api/project/${projectName}/${urlFragment}`, "PUT", {xml_data: toXML(model)});


export const deleteProjectXML = (
        callback: TCallback<IApiSuccess>,
        errorCallback: ErrorCallback,
        projectName: string,
        urlFragment: string,
): Promise<void> => doFetch<IApiSuccess>(callback, errorCallback, `/api/project/${projectName}/${urlFragment}`, "DELETE");


export const copyEntityConfiguration = (
        callback: TCallback<IApiSuccess>,
        errorCallback: ErrorCallback,
        projectName: string,
        entityName: string,
        destinationFolder: string,
): Promise<void> => doFetch<IApiSuccess>(callback, errorCallback, `/api/project/${projectName}/${entityName}`, "PUT", {destination_folder: destinationFolder, method: "CP"});


export const uploadEntityConfiguration = (
        callback: TCallback<IApiSuccess>,
        errorCallback: ErrorCallback,
        projectName: string,
        entityName: string,
        destinationFolder: string,
        method: string,
        address: string,
        user: string,
        password: string,
        isPassphrase: boolean,
): Promise<void> => doFetch<IApiSuccess>(callback, errorCallback, `/api/project/${projectName}/${entityName}`, "PUT", {destination_folder: destinationFolder, method, ssh: {address, user, password, is_passhprase: isPassphrase}});


export const runEntity = (
        callback: TCallback<IApiSuccess>,
        errorCallback: ErrorCallback,
        projectName: string,
        entityName: string,
        method: string,
        address: string,
        user: string,
        password: string,
        isPassphrase: boolean,
): Promise<void> => doFetch<IApiSuccess>(callback, errorCallback, `/api/project/${projectName}/${entityName}`, "PUT", {method, ssh: {address, user, password, is_passhprase: isPassphrase}});
