import React from 'react';
import {useParams, useNavigate} from 'react-router-dom';
import {Formik} from 'formik';
import type {FormikProps, FormikHelpers} from 'formik';

import Box from '@mui/material/Box';
import Button from '@mui/material/Button';
import Toolbar from '@mui/material/Toolbar';
import Typography from '@mui/material/Typography';

import SpacedButton from '../common/SpacedButton';
import RootComponent from '../Model/RootComponent';
import DeployEntityDialog from './DeployEntityDialog';
import EntityAction from './EntityAction';
import NewEntityDialog from './NewEntityDialog';
import PingDialog from './PingDialog';
import PingResultDialog from './PingResultDialog';
import LaunchEntitiesButton from './LaunchEntitiesButton';
// import StatusEntitiesButton from './StatusEntitiesButton';
import StopEntitiesButton from './StopEntitiesButton';

import {
    deleteXML,
    getProject,
    listProjectTemplates,
    pingEntity,
    updateProject,
} from '../../api';
import {useSelector, useDispatch} from '../../redux';
import {newError} from '../../redux/error';
import {clearTemplates} from '../../redux/form';
import {clearModel} from '../../redux/model';
import {openSshDialog, runSshCommand} from '../../redux/ssh';
import {combineActions} from '../../utils/actions';
import type {MutatorCallback} from '../../utils/actions';
import {getXsdName, isComponentElement, isListElement, isParameterElement, newItem} from '../../xsd';
import type {Component, Parameter, List} from '../../xsd';


interface IEntity {
    name: string;
    type: string;
}


type SaveCallback = () => void;


const findMachines = (root?: Component, operation?: (l: List, path: string) => void): List | undefined => {
    if (root) {
        const platformIndex = root.elements.findIndex((e) => isComponentElement(e) && e.element.id === "platform");
        if (platformIndex < 0) { return; }

        const platform = root.elements[platformIndex];
        if (isComponentElement(platform)) {
            const machinesIndex = platform.element.elements.findIndex((e) => isListElement(e) && e.element.id === "machines");
            if (machinesIndex < 0) { return; }

            const machines = platform.element.elements[machinesIndex];
            if (isListElement(machines)) {
                if (operation) {
                    operation(machines.element, `elements.${platformIndex}.element.elements.${machinesIndex}.element`);
                } else {
                    return machines.element;
                }
            }
        }
    }
};


const findEntities = (root?: Component, operation?: (l: List, path: string) => void): List | undefined => {
    if (root) {
        const configurationIndex = root.elements.findIndex((e) => isComponentElement(e) && e.element.id === "configuration");
        if (configurationIndex < 0) { return; }

        const configuration = root.elements[configurationIndex];
        if (isComponentElement(configuration)) {
            const entitiesIndex = configuration.element.elements.findIndex((e) => isListElement(e) && e.element.id === "entities");
            if (entitiesIndex < 0) { return; }

            const entities = configuration.element.elements[entitiesIndex];
            if (isListElement(entities)) {
                if (operation) {
                    operation(entities.element, `elements.${configurationIndex}.element.elements.${entitiesIndex}.element`);
                } else {
                    return entities.element;
                }
            }
        }
    }
};


const applyOnMachinesAndEntities = (root: Component, operation: (l: List, path: string) => void) => {
    findMachines(root, operation);
    findEntities(root, operation);
};


const Project: React.FC<Props> = (props) => {
    const model = useSelector((state) => state.model.model);
    const source = useSelector((state) => state.ping.source);
    const {name} = useParams();
    const dispatch = useDispatch();
    const navigate = useNavigate();

    const [handleNewEntityCreate, setNewEntityCreate] = React.useState<((entity: string, entityType: string) => void) | undefined>(undefined);

    const handleOpen = React.useCallback((root: Component, mutator: MutatorCallback, submitForm: SaveCallback) => {
        setNewEntityCreate(() => (entity: string, entityType: string) => {
            const addNewEntity = (l: List) => {
                let hasError = false;
                l.elements.forEach((c) => {
                    c.elements.forEach((p) => {
                        if (isParameterElement(p)) {
                            if (p.element.id === "entity_name" && p.element.value === entity) {
                                hasError = true;
                            }
                        }
                    });
                });

                if (hasError) {
                    dispatch(newError(`Entity ${entity} already exists in ${l.name}`));
                    return;
                }

                if (l.elements.length < l.maxOccurences) {
                    const newEntity = newItem(l.pattern, l.elements.length);
                    newEntity.elements.forEach((p) => {
                        if (isParameterElement(p)) {
                            if (p.element.id === "entity_name") {
                                p.element.value = entity;
                            }
                            if (p.element.id === "entity_type") {
                                p.element.value = entityType;
                            }
                            if (p.element.type.endsWith("_xsd")) {
                                p.element.value = getXsdName(p.element.id, entityType);
                            }
                        }
                    });
                    return newEntity;
                }
            };
            applyOnMachinesAndEntities(root, (l: List, p: string) => mutator(l, p, addNewEntity));
            submitForm();
        });
    }, [dispatch]);

    const handleClose = React.useCallback(() => {
        setNewEntityCreate(undefined);
    }, []);

    const handleSubmit = React.useCallback((values: Component, helpers: FormikHelpers<Component>) => {
        if (name) {
            dispatch(updateProject({project: name, root: values}));
        }
        helpers.setSubmitting(false);
    }, [dispatch, name]);

    const handleDeleteEntity = React.useCallback((root: Component, mutator: (l: List, path: string) => void) => {
        applyOnMachinesAndEntities(root, mutator);
    }, []);

    const handleSelect = React.useCallback((entity: IEntity | undefined, key: string, xsd: string, xml?: string) => {
        const query: {xsd: string; xml: string;} | {xsd: string;} = xml ? {xsd, xml} : {xsd};
        const url = "/edit/" + name + "/" + key + (entity ? "/" + entity.name : "");
        navigate(url + "?" + new URLSearchParams(query));
    }, [name, navigate]);

    const handleDelete = React.useCallback((entity: string | undefined, key: string) => {
        if (name) {
            const urlFragment = key + (entity == null ? "" : "/" + entity);
            dispatch(deleteXML({project: name, urlFragment}));
        }
    }, [dispatch, name]);

    const handleDownload = React.useCallback((entity?: string) => {
        const form = document.createElement("form") as HTMLFormElement;
        form.method = "post";
        form.action = "/api/project/" + name + (entity == null ? "" : "/" + entity);
        document.body.appendChild(form);
        form.submit();
        document.body.removeChild(form);
    }, [name]);

    const handlePing = React.useCallback((destination: string) => {
        if (name && source) {
            dispatch(runSshCommand({
                action: () => dispatch(pingEntity({
                    project: name,
                    entity: source.name,
                    address: source.address,
                    destination,
                })),
            }));
        }
    }, [dispatch, name, source]);

    const displayAction = React.useCallback((index: number) => {
        const entity = findMachines(model?.root)?.elements[index];
        return (
            <EntityAction
                project={name}
                entity={entity}
                onDownload={handleDownload}
            />
        );
    }, [model, name, handleDownload]);

    const [entityName, entityType]: [Parameter | undefined, Parameter | undefined] = React.useMemo(() => {
        const entity: [Parameter | undefined, Parameter | undefined] = [undefined, undefined];

        findMachines(model?.root, (machines: List) => {
            machines.pattern.elements.forEach((p) => {
                if (isParameterElement(p)) {
                    if (p.element.id === "entity_name") { entity[0] = p.element; }
                    if (p.element.id === "entity_type") { entity[1] = p.element; }
                }
            });
        });

        return entity;
    }, [model]);

    const actions = React.useMemo(() => combineActions([
        {path: ['platform', 'machines'], actions: {onCreate: handleOpen, onDelete: handleDeleteEntity}},
        {path: ['platform', 'machines', 'item'], actions: {onAction: displayAction}},
        {path: ['configuration', 'topology__template'], actions: {onEdit: handleSelect, onRemove: handleDelete}},
        {path: ['configuration', 'entities', 'item', 'profile__template'], actions: {onEdit: handleSelect, onRemove: handleDelete}},
        {path: ['configuration', 'entities', 'item', 'infrastructure__template'], actions: {onEdit: handleSelect, onRemove: handleDelete}},
    ]), [handleOpen, handleDeleteEntity, handleDelete, handleSelect, displayAction]);

    React.useEffect(() => {
        if (name) {
            dispatch(getProject({project: name}));
            dispatch(listProjectTemplates({project: name}));
        }

        return () => {
            dispatch(clearTemplates());
            dispatch(clearModel());
        };
    }, [dispatch, name]);

    return (
        <React.Fragment>
            <Toolbar>
                <Typography variant="h6">Project:&nbsp;</Typography>
                <Typography variant="h6">{name}</Typography>
            </Toolbar>
            {model != null && (
                <Formik enableReinitialize initialValues={model.root} onSubmit={handleSubmit}>
                    {(formik: FormikProps<Component>) => (
                        <RootComponent form={formik} actions={actions} xsd="project.xsd" autosave />
                    )}
                </Formik>
            )}
            {model != null && (<>
                <Box textAlign="center" marginTop="3em" marginBottom="3px">
                    <LaunchEntitiesButton project={model.root} />
                    <StopEntitiesButton project={model.root} />
                    <SpacedButton
                        color="secondary"
                        variant="contained"
                        onClick={() => dispatch(openSshDialog())}
                    >
                        Configure SSH Credentials
                    </SpacedButton>
                    <Button
                        color="secondary"
                        variant="contained"
                        onClick={() => handleDownload()}
                    >
                        Download Project Configuration
                    </Button>
                </Box>
                <DeployEntityDialog />
                <PingDialog onValidate={handlePing} />
                <PingResultDialog />
            </>)}
            {handleNewEntityCreate != null && entityName != null && entityType != null && (
                <NewEntityDialog
                    entityName={entityName}
                    entityType={entityType}
                    onValidate={handleNewEntityCreate}
                    onClose={handleClose}
                />
            )}
        </React.Fragment>
    );
};


interface Props {
}


export default Project;
