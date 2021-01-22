type TCallback<T> = (x: T) => void;
type ErrorCallback = TCallback<string>;


interface IApiError {
    error: string;
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


export const listXSD = (
        callback: TCallback<string[]>,
        errorCallback: ErrorCallback,
): Promise<void> => doFetch<string[]>(callback, errorCallback, "/models/");


export interface IApiSuccess {
    status: string;
}


export const getXSD = (
        callback: TCallback<IXsdContent>,
        errorCallback: ErrorCallback,
        filename: string,
): Promise<void> => doFetch<IXsdContent>(callback, errorCallback, `/model/${filename}`);


export interface IXsdContent {
    content: string;
}


export const sendXML = (
        callback: TCallback<IApiSuccess>,
        errorCallback: ErrorCallback,
        xsdFile: string,
        xml: string,
): Promise<void> => doFetch<IApiSuccess>(callback, errorCallback, `/model/{xsdFile}`, "POST", {xml_data: xml});
