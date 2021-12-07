import React from 'react';
import {useHistory} from 'react-router-dom';

import AppBar from '@material-ui/core/AppBar';
import Tab from '@material-ui/core/Tab';
import Tabs from '@material-ui/core/Tabs';
import Tooltip from '@material-ui/core/Tooltip';

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
import {Model, Component as ComponentType} from '../../xsd/model';

import Component from './Component';


interface Props {
    project: Model;
    projectName: string;
}


interface PanelProps {
    className?: string;
    children?: React.ReactNode;
    index: any;
    value: any;
}


type EntityTypeSetter = (entityType?: string) => void;


const TabPanel = (props: PanelProps) => {
    const {children, value, index, ...other} = props;

    return (
        <div hidden={value !== index} {...other}>
            {value === index && children}
        </div>
    );
};


const Entities = (props: Props) => {
    const {project, projectName} = props;

    const [value, setValue] = React.useState<number>(0);
    const [templates, setTemplates] = React.useState<ITemplatesContent>({});
    const [, setState] = React.useState<object>({});
    const history = useHistory();

    const handleTabChange = React.useCallback((event: React.ChangeEvent<{}>, index: number) => {
        setValue(index);
    }, []);

    const handleUpdate = React.useCallback(() => {
        updateProject(silenceSuccess, sendError, projectName, project);
        setState({});
    }, [projectName, project]);

    const checkModel = React.useCallback((xsd: string, entity: string, setEntityType: EntityTypeSetter) => {
        const dataModel = fromXSD(xsd);
        const onSuccess = (content: IXmlContent) => {
            const model = fromXML(dataModel, content.content);
            const entityComponent = model.root.elements.find(e => e.element.id === "entity");
            if (entityComponent && entityComponent.type === "component") {
                const entityParameter = entityComponent.element.elements.find(e => e.element.id === "entity_type");
                if (entityParameter && entityParameter.type === "parameter") {
                    const entityValue = entityParameter.element.value;
                    setEntityType(entityValue === "" ? undefined : entityValue);
                    return;
                }
            }
            setEntityType(undefined);
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

/*
    const projectComponent = project.root.elements.find(e => e.element.id === "project");
    if (projectComponent == null || projectComponent.type !== "component") {
        return null;
    }
*/
    const components = project.root.getComponents();

    return (
        <>
            <AppBar position="static" color="primary">
                <Tabs
                    value={value}
                    onChange={handleTabChange}
                    indicatorColor="secondary"
                    textColor="inherit"
                    variant="fullWidth"
                >
                    {components.map((c: ComponentType, i: number) => c.description === "" ? (
                        <Tab key={c.id} label={c.name} value={i} />
                    ) : (
                        <Tooltip title={c.description} placement="top" key={c.id}>
                            <Tab key={c.id} label={c.name} value={i} />
                        </Tooltip>
                    ))}
                </Tabs>
            </AppBar>
            {components.map((c: ComponentType, i: number) => (
                <TabPanel key={i} value={value} index={i}>
                    <Component
                        component={c}
                        templates={templates}
                        onSelect={handleSelect}
                        onEdit={handleEdit}
                        onDelete={handleDelete}
                        onDownload={handleDownload}
                        forceUpdate={handleUpdate}
                    />
                </TabPanel>
            ))}
        </>
    );
};


export default Entities;
