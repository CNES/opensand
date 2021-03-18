import React from 'react';
import {RouteComponentProps} from 'react-router-dom';

import Box from '@material-ui/core/Box';
import Typography from '@material-ui/core/Typography';

import {getXSD, getProjectXML, updateProjectXML, silenceSuccess, IXsdContent, IXmlContent} from '../../api';
import {sendError} from '../../utils/dispatcher';
import {fromXSD, fromXML} from '../../xsd/parser';
import {Model as ModelType} from '../../xsd/model';

import Model from './Model';


interface RouteParams {
    project: string;
    model: string;
    entity?: string;
}


type Props = RouteComponentProps<RouteParams>;


const Editor = (props: Props) => {
    const [model, changeModel] = React.useState<ModelType | undefined>(undefined);

    const [projectName, urlFragment, xsd, template] = React.useMemo(() => {
        const urlParams = new URLSearchParams(props.location.search);
        const {project, model, entity} = props.match.params;
        const fragment = model + (entity == null ? "" : "/" + entity);
        return [project, fragment, urlParams.get("xsd"), urlParams.get("xml")];
    }, [props.match.params, props.location.search]);

    const loadModel = React.useCallback((content: IXsdContent) => {
        const dataModel = fromXSD(content.content);
        const url = template == null ? urlFragment : "template/" + xsd + "/" + template;
        console.log(url);
        const onSaveError = (error: string) => sendError("Configuration has not been automatically saved: " + error);
        const onError = (error: string) => {sendError(error); changeModel(dataModel);};
        const onSuccess = (content: IXmlContent) => {
            const newModel = fromXML(dataModel, content.content);
            changeModel(newModel);
            updateProjectXML(silenceSuccess, onSaveError, projectName, urlFragment, newModel);
        }
        getProjectXML(onSuccess, onError, projectName, url);
    }, [changeModel, projectName, urlFragment, xsd, template]);

    React.useEffect(() => {
        if (xsd != null) {
            getXSD(loadModel, sendError, xsd);
        }
        return () => {changeModel(undefined);}
    }, [loadModel, xsd]);

    if (model == null) {
        return <Box m={3}><Typography align="center">Model does not exist!</Typography></Box>;
    }

    return (
        <Model
            model={model}
            xsd={xsd}
            projectName={projectName}
            urlFragment={urlFragment}
        />
    );
};


export default Editor;
