import React from 'react';
import {RouteComponentProps} from 'react-router-dom';

// import Tab from '@material-ui/core/Tab';
// import Tabs from '@material-ui/core/Tabs';
import Toolbar from '@material-ui/core/Toolbar';
import Typography from '@material-ui/core/Typography';

import {getProjectModel, getProject, IXsdContent, IXmlContent} from '../../api';
import {sendError} from '../../utils/dispatcher';
import {Model} from '../../xsd/model';
import {fromXSD, fromXML} from '../../xsd/parser';

// import Templates from './Templates';
import Entities from './Entities';


interface Props extends RouteComponentProps<{name: string;}> {
}


const Project = (props: Props) => {
    const projectName = props.match.params.name;
    // const [value, setValue] = React.useState<number>(0);
    const [model, changeModel] = React.useState<Model | undefined>(undefined);

/*
    const handleChange = React.useCallback((event: React.ChangeEvent<{}>, index: number) => {
        setValue(index);
    }, [setValue]);
*/
    const loadProject = React.useCallback((content: IXsdContent) => {
        const newModel = fromXSD(content.content);
        const onSuccess = (xml: IXmlContent) => {
            changeModel(fromXML(newModel, xml.content));
        };
        getProject(onSuccess, sendError, projectName);
    }, [changeModel, projectName]);

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
            {model != null && <Entities project={model} projectName={projectName} />}
        </React.Fragment>
    );
/*

    return (
        <React.Fragment>
            <AppBar position="static" color="primary">
                <Tabs
                    value={value}
                    onChange={handleChange}
                    indicatorColor="secondary"
                    textColor="inherit"
                    variant="fullWidth"
                >
                    <Tab label="Entities" value={0} />
                    <Tab label="Templates" value={1} />
                </Tabs>
            </AppBar>
            {value === 0 && model != null && <Entities project={model} projectName={projectName} />}
            {value === 1 && model != null && <Templates project={model} projectName={projectName} />}
        </React.Fragment>
    );
*/
};


export default Project;
