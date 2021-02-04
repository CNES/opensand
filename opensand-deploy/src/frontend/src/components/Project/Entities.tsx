import React from 'react';
import {useHistory} from 'react-router-dom';

import {updateProject, listProjectTemplates, ITemplatesContent} from '../../api';
import {Model, Component, Parameter} from '../../xsd/model';

import Components from './Component';


interface Props {
    project: Model;
}


const Entities = (props: Props) => {
    const projectComponent = props.project.root.children.find((c: Component) => c.id === "project");
    const projectName = projectComponent?.parameters.find((p: Parameter) => p.id === "name")?.value;
    const [templates, setTemplates] = React.useState<ITemplatesContent>({});
    const [, setState] = React.useState<object>({});
    const history = useHistory();

    const handleUpdate = React.useCallback(() => {
        if (projectName != null) {
            updateProject(console.log, console.log, projectName, props.project);
        }
        setState({});
    }, [setState, projectName, props.project]);

    const handleEdit = React.useCallback((entity: string | null, model: string, xsd: string, xml?: string) => {
        if (projectName != null) {
            const url = entity == null ? model : model + "/" + entity;
            const file = xml == null ? url : "template/" + xsd + "/" + xml;
            history.push({
                pathname: "/edit/" + projectName,
                search: "?url=" + url + "&xsd=" + xsd + "&xml=" + file,
            });
        }
    }, [history, projectName]);

    React.useEffect(() => {
        if (projectName != null) {
            listProjectTemplates(setTemplates, console.log, projectName);
        }
        return () => {setTemplates({});};
    }, [setTemplates, projectName]);

    if (projectComponent == null) {
        return null;
    }

    return (
        <Components
            component={projectComponent}
            templates={templates}
            onEdit={handleEdit}
            forceUpdate={handleUpdate}
        />
    );
};


export default Entities;
