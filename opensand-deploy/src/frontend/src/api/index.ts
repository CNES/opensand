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


export interface IXsdContent {
    content: string;
}


export interface IXmlContent {
    content: string;
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


export const listProjects = (
        callback: TCallback<string[]>,
        errorCallback: ErrorCallback,
): Promise<void> => doFetch<string[]>(callback, errorCallback, "/api/projects");


export const getProjectModel = (
        callback: TCallback<IXsdContent>,
        errorCallback: ErrorCallback,
): Promise<void> => doFetch<IXsdContent>(callback, errorCallback, "/api/project");


export const getProject = (
        callback: TCallback<IXmlContent>,
        errorCallback: ErrorCallback,
        projectName: string,
): Promise<void> => doFetch<IXmlContent>(callback, errorCallback, "/api/project/" + projectName);


export const updateProject = (
        callback: TCallback<IApiSuccess>,
        errorCallback: ErrorCallback,
        projectName: string,
        project: Model,
): Promise<void> => doFetch<IApiSuccess>(callback, errorCallback, "/api/project/" + projectName, "PUT", {xml_data: toXML(project)});


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


export const getProjectTopology = (
        callback: TCallback<IXmlContent>,
        errorCallback: ErrorCallback,
        projectName: string,
): Promise<void> => doFetch<IXmlContent>(callback, errorCallback, "/api/project/" + projectName + "/topology");


export const updateProjectTopology = (
        callback: TCallback<IApiSuccess>,
        errorCallback: ErrorCallback,
        projectName: string,
        topology: Model,
): Promise<void> => doFetch<IApiSuccess>(callback, errorCallback, "/api/project/" + projectName + "/topology", "PUT", {xml_data: toXML(topology)});


export const getProjectInfrastructure = (
        callback: TCallback<IXmlContent>,
        errorCallback: ErrorCallback,
        projectName: string,
        entityName: string,
): Promise<void> => doFetch<IXmlContent>(callback, errorCallback, "/api/project/" + projectName + "/infrastructure/" + entityName);


export const updateProjectInfrastructure = (
        callback: TCallback<IApiSuccess>,
        errorCallback: ErrorCallback,
        projectName: string,
        entityName: string,
        infrastructure: Model,
): Promise<void> => doFetch<IApiSuccess>(callback, errorCallback, "/api/project/" + projectName + "/infrastructure/" + entityName, "PUT", {xml_data: toXML(infrastructure)});


export const deleteProjectInfrastructure = (
        callback: TCallback<IApiSuccess>,
        errorCallback: ErrorCallback,
        projectName: string,
        entityName: string,
): Promise<void> => doFetch<IApiSuccess>(callback, errorCallback, "/api/project/" + projectName + "/infrastructure/" + entityName, "DELETE");


export const getProjectProfile = (
        callback: TCallback<IXmlContent>,
        errorCallback: ErrorCallback,
        projectName: string,
        entityName: string,
): Promise<void> => doFetch<IXmlContent>(callback, errorCallback, "/api/project/" + projectName + "/profile/" + entityName);


export const updateProjectProfile = (
        callback: TCallback<IApiSuccess>,
        errorCallback: ErrorCallback,
        projectName: string,
        entityName: string,
        profile: Model,
): Promise<void> => doFetch<IApiSuccess>(callback, errorCallback, "/api/project/" + projectName + "/profile/" + entityName, "PUT", {xml_data: toXML(profile)});


export const deleteProjectProfile = (
        callback: TCallback<IApiSuccess>,
        errorCallback: ErrorCallback,
        projectName: string,
        entityName: string,
): Promise<void> => doFetch<IApiSuccess>(callback, errorCallback, "/api/project/" + projectName + "/profile/" + entityName, "DELETE");


export const listProjectTemplates = (
        callback: TCallback<ITemplatesContent>,
        errorCallback: ErrorCallback,
        projectName: string,
): Promise<void> => doFetch<ITemplatesContent>(callback, errorCallback, "/api/project/" + projectName + "/templates");


export const getProjectTemplate = (
        callback: TCallback<IXmlContent>,
        errorCallback: ErrorCallback,
        projectName: string,
        templateName: string,
        fileName: string,
): Promise<void> => doFetch<IXmlContent>(callback, errorCallback, "/api/project/" + projectName + "/template/" + templateName + "/" + fileName);


export const updateProjectTemplate = (
        callback: TCallback<IApiSuccess>,
        errorCallback: ErrorCallback,
        projectName: string,
        templateName: string,
        fileName: string,
        template: Model,
): Promise<void> => doFetch<IApiSuccess>(callback, errorCallback, "/api/project/" + projectName + "/template/" + templateName + "/" + fileName, "PUT", {xml_data: toXML(template)});


export const deleteProjectTemplate = (
        callback: TCallback<IApiSuccess>,
        errorCallback: ErrorCallback,
        projectName: string,
        templateName: string,
        fileName: string,
): Promise<void> => doFetch<IApiSuccess>(callback, errorCallback, "/api/project/" + projectName + "/template/" + templateName + "/" + fileName, "DELETE");


export const listXSD = (
        callback: TCallback<string[]>,
        errorCallback: ErrorCallback,
): Promise<void> => doFetch<string[]>(callback, errorCallback, "/api/models");


export const getXSD = (
        callback: TCallback<IXsdContent>,
        errorCallback: ErrorCallback,
        filename: string,
): Promise<void> => doFetch<IXsdContent>(callback, errorCallback, `/api/model/${filename}`);
