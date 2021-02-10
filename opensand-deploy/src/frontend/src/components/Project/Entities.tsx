import React from 'react';
import {useHistory} from 'react-router-dom';

import {updateProject, deleteProjectXML, listProjectTemplates, silenceSuccess, ITemplatesContent} from '../../api';
import {sendError} from '../../utils/dispatcher';
import {Model, Component} from '../../xsd/model';

import Components from './Component';


interface Props {
    project: Model;
    projectName: string;
}


const Entities = (props: Props) => {
    const {project, projectName} = props;
    const [templates, setTemplates] = React.useState<ITemplatesContent>({});
    const [, setState] = React.useState<object>({});
    const history = useHistory();

    const handleUpdate = React.useCallback(() => {
        updateProject(silenceSuccess, sendError, projectName, project);
        setState({});
    }, [setState, projectName, project]);

    const handleEdit = React.useCallback((entity: string | null, model: string, xsd: string, xml?: string) => {
        handleUpdate();
        history.push({
            pathname: "/edit/" + projectName + "/" + model + (entity == null ? "" : "/" + entity),
            search: "?xsd=" + xsd + (xml == null ? "" : "&xml=" + xml),
        });
    }, [history, projectName, handleUpdate]);

    const handleDelete = React.useCallback((entity: string | null, model: string) => {
        handleUpdate();
        const url = model + (entity == null ? "" : "/" + entity);
        deleteProjectXML(silenceSuccess, sendError, projectName, url);
    }, [projectName, handleUpdate]);

    React.useEffect(() => {
        listProjectTemplates(setTemplates, sendError, projectName);
        return () => {setTemplates({});};
    }, [setTemplates, projectName]);

    const projectComponent = project.root.children.find((c: Component) => c.id === "project");
    if (projectComponent == null) {
        return null;
    }

    return (
        <Components
            component={projectComponent}
            templates={templates}
            onEdit={handleEdit}
            onDelete={handleDelete}
            forceUpdate={handleUpdate}
        />
    );
};


export default Entities;
