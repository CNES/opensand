import React from 'react';
import {RouteComponentProps} from 'react-router-dom';

import AppBar from '@material-ui/core/AppBar';
import Tab from '@material-ui/core/Tab';
import Tabs from '@material-ui/core/Tabs';

import {getProjectModel, getProject, IXsdContent, IXmlContent} from '../../api';
import {Model} from '../../xsd/model';
import {fromXSD, fromXML} from '../../xsd/parser';

import Templates from './Templates';
import Entities from './Entities';


interface Props extends RouteComponentProps<{name: string;}> {
}


const Project = (props: Props) => {
    const projectName = props.match.params.name;
    const [value, setValue] = React.useState<number>(0);
    const [model, changeModel] = React.useState<Model | undefined>(undefined);

    const handleChange = React.useCallback((event: React.ChangeEvent<{}>, index: number) => {
        setValue(index);
    }, [setValue]);

    const loadProject = React.useCallback((content: IXsdContent) => {
        const newModel = fromXSD(content.content);
        getProject((xml: IXmlContent) => {changeModel(fromXML(newModel, xml.content));}, console.log, projectName);
    }, [changeModel, projectName]);

    React.useEffect(() => {
        getProjectModel(loadProject, console.log);
        return () => {changeModel(undefined);};
    }, [loadProject]);

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
            {value === 0 && model != null && <Entities project={model} />}
            {value === 1 && model != null && <Templates project={model} />}
        </React.Fragment>
    );
};


export default Project;
