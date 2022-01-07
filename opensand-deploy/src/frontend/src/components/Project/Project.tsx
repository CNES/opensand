import React from 'react';
import {RouteComponentProps, useHistory} from 'react-router-dom';

import Box from '@material-ui/core/Box';
import Button from '@material-ui/core/Button';
import Toolbar from '@material-ui/core/Toolbar';
import Typography from '@material-ui/core/Typography';

import {
    deleteProjectXML,
    getProject,
    getProjectModel,
    getProjectXML,
    getXSD,
    pingEntity,
    silenceSuccess,
    updateProject,
    updateProjectXML,
    IApiSuccess,
    IPingSuccess,
    IXsdContent,
    IXmlContent,
} from '../../api';
import {combineActions} from '../../utils/actions';
import {sendError} from '../../utils/dispatcher';
import {Model, Parameter, List} from '../../xsd/model';
import {isComponentElement, isListElement, isParameterElement} from '../../xsd/model';
import {fromXSD, fromXML} from '../../xsd/parser';

import RootComponent from '../Model/RootComponent';
import DeployEntityDialog from './DeployEntityDialog';
import EntityAction from './EntityAction';
import NewEntityDialog from './NewEntityDialog';
import PingDialog from './PingDialog';
import PingResultDialog from './PingResultDialog';


interface Props extends RouteComponentProps<{name: string;}> {
}


interface IEntity {
    name: string;
    type: string;
}


type ActionCallback = (password: string, isPassphrase: boolean) => void;
type PingActionCallback = (destination: string) => void;


const addNewEntity = (l: List, entity: string, entityType: string) => {
    let hasError = false;
    l.elements.forEach((c) => {
        c.elements.forEach((p) => {
            if (p.type === "parameter" && p.element.id === "entity_name" && p.element.value === entity) {
                hasError = true;
            }
        });
    });

    if (hasError) {
        sendError(`Entity ${entity} already exists in ${l.name}`);
        return;
    }

    const newEntity = l.addItem();
    if (newEntity != null) {
        newEntity.elements.forEach((p) => {
            if (p.type === "parameter") {
                if (p.element.id === "entity_name") {
                    p.element.value = entity;
                }
                if (p.element.id === "entity_type") {
                    p.element.value = entityType;
                }
            }
        });
    }
};


const findMachines = (model: Model): List | undefined => {
    const platform = model.root.elements.find((e) => e.type === "component" && e.element.id === "platform");
    if (isComponentElement(platform)) {
        const machines = platform.element.elements.find((e) => e.type === "list" && e.element.id === "machines");
        if (isListElement(machines)) {
            return machines.element;
        }
    }
};


const findEntities = (model: Model): List | undefined => {
    const configuration = model.root.elements.find((e) => e.type === "component" && e.element.id === "configuration");
    if (isComponentElement(configuration)) {
        const entities = configuration.element.elements.find((e) => e.type === "list" && e.element.id === "entities");
        if (isListElement(entities)) {
            return entities.element;
        }
    }
};


const applyOnMachinesAndEntities = (model: Model, operation: (l: List) => void) => {
    const machines = findMachines(model);
    if (machines) { operation(machines); }

    const entities = findEntities(model);
    if (entities) { operation(entities); }
};


const Project = (props: Props) => {
    const projectName = props.match.params.name;
    const history = useHistory();

    const [timeout, changeTimeout] = React.useState<NodeJS.Timeout|undefined>(undefined);
    const [model, changeModel] = React.useState<Model | undefined>(undefined);
    const [open, setOpen] = React.useState<boolean>(false);
    const [, modelChanged] = React.useState<object>({});
    const [action, setAction] = React.useState<ActionCallback | undefined>(undefined);
    const [pingResult, setPingResult] = React.useState<string | undefined>(undefined);
    const [pingDestinations, setPingDestinations] = React.useState<string[]>([]);
    const [pingAction, setPingAction] = React.useState<PingActionCallback | undefined>(undefined);

    const handleNewEntityOpen = React.useCallback(() => {
        setOpen(true);
    }, []);

    const handleNewEntityClose = React.useCallback(() => {
        setOpen(false);
    }, []);

    const handleActionClose = React.useCallback(() => {
        setAction(undefined);
    }, []);

    const handlePingClose = React.useCallback(() => {
        setPingAction(undefined);
        setPingResult(undefined);
    }, []);

    const saveModel = React.useCallback(() => {
        if (model) updateProject(silenceSuccess, sendError, projectName, model);
        changeTimeout(undefined);
    }, [projectName, model]);

    const refreshModel = React.useCallback((delay: number = 1500) => {
        if (timeout != null) { clearTimeout(timeout); }
        modelChanged({});
        changeTimeout(setTimeout(saveModel, delay));
    }, [timeout, saveModel]);

    const handleNewEntityCreate = React.useCallback((entity: string, entityType: string) => {
        if (!model) {
            return;
        }

        applyOnMachinesAndEntities(model, (l: List) => addNewEntity(l, entity, entityType));
        setOpen(false);
        refreshModel(0);
    }, [model, refreshModel]);

    const handleDeleteEntity = React.useCallback(() => {
        if (!model) {
            return;
        }

        applyOnMachinesAndEntities(model, (l: List) => l.removeItem());
        refreshModel(0);
    }, [model, refreshModel]);

    const handleSelect = React.useCallback((entity: IEntity | undefined, key: string, xsd: string, xml?: string) => {
        refreshModel(0);
        history.push({
            pathname: "/edit/" + projectName + "/" + key + (entity == null ? "" : "/" + entity.name),
            search: "?xsd=" + xsd + (xml == null ? "" : "&xml=" + xml),
        });
    }, [projectName, refreshModel, history]);

    const forceEditEntityType = React.useCallback((content: IXsdContent, entity: IEntity, key: string, xsd: string, xml?: string) => {
        const saveURL = key + "/" + entity.name;
        const loadURL = xml != null ? `template/${xsd}/${xml}` : saveURL;

        const onSaveSuccess = (status: IApiSuccess) => {
            handleSelect(entity, key, xsd);
        };

        const onLoadSuccess = (data: IXmlContent) => {
            const dataModel = fromXML(fromXSD(content.content), data.content);
            const entityComponent = dataModel.root.elements.find((e) => e.element.id === "entity");
            if (isComponentElement(entityComponent)) {
                const entityParameter = entityComponent.element.elements.find((e) => e.element.id === "entity_type");
                if (isParameterElement(entityParameter)) {
                    entityParameter.element.value = entity.type;
                    updateProjectXML(onSaveSuccess, sendError, projectName, saveURL, dataModel);
                }
            }
        };

        getProjectXML(onLoadSuccess, sendError, projectName, loadURL);
    }, [projectName, handleSelect]);

    const handleSelectForceEntity = React.useCallback((entity: IEntity | undefined, key: string, xsd: string, xml?: string) => {
        if (entity == null) { return; }
        const onSuccess = (content: IXsdContent) => forceEditEntityType(content, entity, key, xsd, xml);
        getXSD(onSuccess, sendError, xsd);
    }, [forceEditEntityType]);

    const handleDelete = React.useCallback((entity: string | undefined, key: string) => {
        refreshModel(0);
        const url = key + (entity == null ? "" : "/" + entity);
        deleteProjectXML(silenceSuccess, sendError, projectName, url);
    }, [projectName, refreshModel]);

    const loadProject = React.useCallback((content: IXsdContent) => {
        const newModel = fromXSD(content.content);
        const onSuccess = (xml: IXmlContent) => {
            changeModel(fromXML(newModel, xml.content));
        };
        getProject(onSuccess, sendError, projectName);
    }, [changeModel, projectName]);

    const handleDownload = React.useCallback((entity?: string) => {
        const form = document.createElement("form") as HTMLFormElement;
        form.method = "post";
        form.action = "/api/project/" + projectName + (entity == null ? "" : "/" + entity);
        document.body.appendChild(form);
        form.submit();
        document.body.removeChild(form);
    }, [projectName]);

    const handlePingResult = React.useCallback((result: IPingSuccess) => {
        const {ping} = result;
        setPingResult(ping);
    }, []);

    const handlePingDestinations = React.useCallback((entity: string, address: string, destinations: string[]) => {
        setPingDestinations(destinations);
        setPingAction(() => (destination: string) => setAction(() => (password: string, isPassphrase: boolean) => (
            pingEntity(handlePingResult, sendError, projectName, entity, destination, address, password, isPassphrase)
        )));
    }, [projectName, handlePingResult]);

    const displayAction = React.useCallback((index: number) => {
        const machines = model ? findMachines(model) : undefined;
        const entity = machines?.elements[index];
        return (
            <EntityAction
                project={projectName}
                entity={entity}
                onDownload={handleDownload}
                setAction={setAction}
                setPingDestinations={handlePingDestinations}
            />
        );
    }, [model, projectName, handleDownload, handlePingDestinations]);

    const [entityName, entityType]: [Parameter | undefined, Parameter | undefined] = React.useMemo(() => {
        const entity: [Parameter | undefined, Parameter | undefined] = [undefined, undefined];

        if (model) {
            const machines = findMachines(model);
            if (machines) {
                machines.pattern.elements.forEach((p) => {
                    if (isParameterElement(p)) {
                        if (p.element.id === "entity_name") { entity[0] = p.element; }
                        if (p.element.id === "entity_type") { entity[1] = p.element; }
                    }
                });
            }
        }

        return entity;
    }, [model]);

    const actions = React.useMemo(() => combineActions([
        {path: ['platform', 'machines'], actions: {onCreate: handleNewEntityOpen, onDelete: handleDeleteEntity}},
        {path: ['platform', 'machines', 'item'], actions: {onAction: displayAction}},
        {path: ['configuration', 'topology'], actions: {onEdit: handleSelect, onRemove: handleDelete}},
        {path: ['configuration', 'entities', 'item', 'profile'], actions: {onEdit: handleSelect, onRemove: handleDelete}},
        {path: ['configuration', 'entities', 'item', 'infrastructure'], actions: {onEdit: handleSelectForceEntity, onRemove: handleDelete}},
    ]), [handleNewEntityOpen, handleDeleteEntity, handleDelete, handleSelect, handleSelectForceEntity, displayAction]);

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
            {model != null && <RootComponent root={model.root} modelChanged={refreshModel} actions={actions} />}
            {model != null && (
                <Box textAlign="center" marginTop="3em" marginBottom="3px">
                    <Button
                        color="secondary"
                        variant="contained"
                        onClick={() => handleDownload()}
                    >
                        Download Project Configuration
                    </Button>
                </Box>
            )}
            <DeployEntityDialog
                open={Boolean(action)}
                onValidate={action}
                onClose={handleActionClose}
            />
            <PingDialog
                open={Boolean(pingAction)}
                destinations={pingDestinations}
                onValidate={pingAction}
                onClose={handlePingClose}
            />
            <PingResultDialog
                open={pingResult != null}
                content={pingResult}
                onClose={handlePingClose}
            />
            {entityName != null && entityType != null && (
                <NewEntityDialog
                    open={open}
                    entityName={entityName}
                    entityType={entityType}
                    onValidate={handleNewEntityCreate}
                    onClose={handleNewEntityClose}
                />
            )}
        </React.Fragment>
    );
};


export default Project;
