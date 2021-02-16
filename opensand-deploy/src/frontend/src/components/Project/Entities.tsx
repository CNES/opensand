import React from 'react';
import {useHistory} from 'react-router-dom';

import {
    getXSD,
    updateProject,
    listProjectTemplates,
    getProjectXML,
    deleteProjectXML,
    silenceSuccess,
    IXsdContent,
    IXmlContent,
    ITemplatesContent,
} from '../../api';
import {sendError} from '../../utils/dispatcher';
import {fromXSD, fromXML} from '../../xsd/parser';
import {Model, Component, Parameter} from '../../xsd/model';

import Components from './Component';


interface Props {
    project: Model;
    projectName: string;
}


type EntityTypeSetter = (entityType?: string) => void;


const Entities = (props: Props) => {
    const {project, projectName} = props;
    const [templates, setTemplates] = React.useState<ITemplatesContent>({});
    const [, setState] = React.useState<object>({});
    const history = useHistory();

    const handleUpdate = React.useCallback(() => {
        updateProject(silenceSuccess, sendError, projectName, project);
        setState({});
    }, [setState, projectName, project]);

    const checkModel = React.useCallback((xsd: string, entity: string, setEntityType: EntityTypeSetter) => {
        const dataModel = fromXSD(xsd);
        const onSuccess = (content: IXmlContent) => {
            const model = fromXML(dataModel, content.content);
            const entityComponent = model.root.children.find((c: Component) => c.id === "entity");
            const entityParameter = entityComponent?.parameters.find((p: Parameter) => p.id === "entity_type");
            const entityValue = entityParameter?.value;
            setEntityType(entityValue == null || entityValue === "" ? undefined : entityValue);
        }
        const onError = (error: string) => {
            setEntityType(undefined);
            sendError(error);
        }
        getProjectXML(onSuccess, onError, projectName, "infrastructure/" + entity);
    }, [projectName]);

    const handleSelect = React.useCallback((entity: string, setEntityType: EntityTypeSetter) => {
        const onSuccess = (content: IXsdContent) => checkModel(content.content, entity, setEntityType);
        getXSD(onSuccess, sendError, "infrastructure.xsd");
    }, [checkModel]);

    const handleDownload = React.useCallback((entity: string | null) => {
        const form = document.createElement("form") as HTMLFormElement;
        form.method = "post";
        form.action = "/api/project/" + projectName + (entity == null ? "" : "/" + entity);
        document.body.appendChild(form);
        form.submit();
        document.body.removeChild(form);
    }, [projectName]);

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
            onSelect={handleSelect}
            onEdit={handleEdit}
            onDelete={handleDelete}
            onDownload={handleDownload}
            forceUpdate={handleUpdate}
        />
    );
};


export default Entities;
