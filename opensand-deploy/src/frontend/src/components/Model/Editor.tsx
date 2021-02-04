import React from 'react';
import {RouteComponentProps} from 'react-router-dom';

import Box from '@material-ui/core/Box';
import Typography from '@material-ui/core/Typography';

import {getXSD, getProjectXML, IXsdContent, IXmlContent} from '../../api';
import {fromXSD, fromXML} from '../../xsd/parser';
import {Model as ModelType} from '../../xsd/model';

import Model from './Model';


interface Props extends RouteComponentProps<{name: string;}> {
}


const Editor = (props: Props) => {
    const projectName = props.match.params.name;
    const queryString = props.location.search;

    const [model, changeModel] = React.useState<ModelType | undefined>(undefined);

    const [modelName, xmlFilepath, urlFragment] = React.useMemo(() => {
        const urlParams = new URLSearchParams(queryString);
        const updateUrl = urlParams.get("url");
        return [urlParams.get("xsd"), urlParams.get("xml") || updateUrl, updateUrl];
    }, [queryString]);

    const loadModel = React.useCallback((content: IXsdContent) => {
        const dataModel = fromXSD(content.content);
        if (xmlFilepath != null) {
            const onSuccess = (content: IXmlContent) => changeModel(fromXML(dataModel, content.content));
            const onError = (error: string) => {console.log(error); changeModel(dataModel);};
            getProjectXML(onSuccess, onError, projectName, xmlFilepath);
        } else {
            changeModel(dataModel);
        }
    }, [changeModel, projectName, xmlFilepath]);

    React.useEffect(() => {
        if (modelName != null) {
            getXSD(loadModel, console.log, modelName);
        }
        return () => {changeModel(undefined);}
    }, [loadModel, modelName]);

    if (model == null) {
        return <Box m={3}><Typography align="center">Model does not exist!</Typography></Box>;
    }

    return <Model model={model} projectName={projectName} urlFragment={urlFragment || ""}/>;
};


export default Editor;
