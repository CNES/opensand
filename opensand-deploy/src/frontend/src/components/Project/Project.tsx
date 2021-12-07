import React from 'react';
import {RouteComponentProps} from 'react-router-dom';

import Toolbar from '@material-ui/core/Toolbar';
import Typography from '@material-ui/core/Typography';

import {getProjectModel, getProject, IXsdContent, IXmlContent} from '../../api';
import {sendError} from '../../utils/dispatcher';
import {Model} from '../../xsd/model';
import {fromXSD, fromXML} from '../../xsd/parser';

import RootComponent from '../Model/RootComponent';


interface Props extends RouteComponentProps<{name: string;}> {
}


const Project = (props: Props) => {
    const projectName = props.match.params.name;
    const [model, changeModel] = React.useState<Model | undefined>(undefined);
    const [, modelChanged] = React.useState<object>({});

    const loadProject = React.useCallback((content: IXsdContent) => {
        const newModel = fromXSD(content.content);
        const onSuccess = (xml: IXmlContent) => {
            changeModel(fromXML(newModel, xml.content));
        };
        getProject(onSuccess, sendError, projectName);
    }, [changeModel, projectName]);

    const refreshModel = React.useCallback(() => {
        modelChanged({});
    }, []);

    React.useEffect(() => {
        getProjectModel(loadProject, sendError);
        return () => {changeModel(undefined);};
    }, [loadProject]);

    return (
        <React.Fragment>
            <Toolbar>
                <Typography variant="h6">Project:&nbsp;</Typography>
                <Typography variant="h6">{projectName}</Typography>
            </Toolbar>
            {model != null && <RootComponent root={model.root} modelChanged={refreshModel} />}
        </React.Fragment>
    );
};


export default Project;
