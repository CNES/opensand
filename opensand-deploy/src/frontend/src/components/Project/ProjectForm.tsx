import React from 'react';

import Avatar from '@mui/material/Avatar';
import Box from '@mui/material/Box';
import Button from '@mui/material/Button';
import Card from '@mui/material/Card';
import CardActions from '@mui/material/CardActions';
import CardHeader from '@mui/material/CardHeader';
import CardMedia from '@mui/material/CardMedia';
import Dialog from '@mui/material/Dialog';
import DialogActions from '@mui/material/DialogActions';
import DialogContent from '@mui/material/DialogContent';
import DialogContentText from '@mui/material/DialogContentText';
import DialogTitle from '@mui/material/DialogTitle';
import Grid from '@mui/material/Grid';

import AddIcon from '@mui/icons-material/NoteAdd';
import AddImage from './add.jpg';

import EntityCard from './EntityCard';
import Parameter, {XsdParameter} from '../Model/Parameter';

import type {IXsdAction, IProjectAction} from '../../utils/actions';
import {useProjectMutators} from '../../utils/hooks';
import {isComponentElement, isParameterElement} from '../../xsd';
import type {Component as ComponentType, List as ListType} from '../../xsd';


const findProjectName = (root: ComponentType): React.ReactNode => {
    const platformIndex = root.elements.findIndex((e) => isComponentElement(e) && e.element.id === "platform");
    const platform = root.elements[platformIndex];
    if (isComponentElement(platform)) {
        const projectIndex = platform.element.elements.findIndex((e) => isParameterElement(e) && e.element.id === "project");
        const project = platform.element.elements[projectIndex];
        if (isParameterElement(project)) {
            return (
                <Parameter
                    parameter={project.element}
                    readOnly={false}
                    prefix={`elements.${platformIndex}.element.elements.${projectIndex}.element`}
                />
            );
        }
    }

    return null;
};


const findTopologyTemplate = (root: ComponentType, action: IXsdAction): React.ReactNode => {
    const configurationIndex = root.elements.findIndex((e) => isComponentElement(e) && e.element.id === "configuration");
    const configuration = root.elements[configurationIndex];
    if (isComponentElement(configuration)) {
        const topologyIndex = configuration.element.elements.findIndex((e) => isParameterElement(e) && e.element.id === "topology__template");
        const topology = configuration.element.elements[topologyIndex];
        if (isParameterElement(topology)) {
            return (
                <XsdParameter
                    parameter={topology.element}
                    readOnly={false}
                    name={`elements.${configurationIndex}.element.elements.${topologyIndex}.element.value`}
                    actions={action}
                />
            );
        }
    }

    return null;
};


const ProjectForm: React.FC<Props> = (props) => {
    const {projectName, root, machines, entities, onDownload, machinesActions, templatesActions} = props;

    const [selectedIndex, setIndex] = React.useState<number>();

    const [addMachine, removeMachine] = useProjectMutators(machinesActions);

    const onClose = React.useCallback(() => {
        setIndex(undefined);
    }, []);

    const onRemove = React.useCallback(() => {
        if (selectedIndex != null) {
            removeMachine(selectedIndex);
        }
        onClose();
    }, [selectedIndex, removeMachine, onClose]);

    const selectedName = React.useMemo(() => {
        if (selectedIndex != null) {
            const machine = machines.elements[selectedIndex];
            const name = machine.elements.find((e) => e.element.id === "entity_name");
            if (isParameterElement(name)) {
                return name.element.value;
            }
        }
    }, [selectedIndex, machines.elements]);

    return (
        <Box p={2}>
            {findProjectName(root)}
            {findTopologyTemplate(root, templatesActions)}
            <Grid container spacing={1}>
                {machines.elements.map((machine: ComponentType, i: number) => {
                    const entity: ComponentType = entities.elements[i];
                    return (
                        <Grid item xs={12} sm={6} lg={4} xl={3} key={i}>
                            <EntityCard
                                index={i}
                                project={projectName}
                                machine={machine}
                                entity={entity}
                                onDownload={onDownload}
                                removeMachine={setIndex}
                                templatesActions={templatesActions}
                            />
                        </Grid>
                    );
                })}
                <Grid item xs={12} sm={6} lg={4} xl={3}>
                    <Card variant="outlined">
                        <CardHeader
                            avatar={<Avatar><AddIcon /></Avatar>}
                            title="Add a new machine"
                            subheader="Complement your platform with a new entity"
                        />
                        <CardMedia
                            component="img"
                            image={AddImage}
                            alt="Add icon"
                            height="180"
                            sx={{objectFit: "contain"}}
                        />
                        <CardActions>
                            <Button
                                color="success"
                                variant="outlined"
                                startIcon={<AddIcon />}
                                onClick={addMachine}
                            >
                                Add New
                            </Button>
                        </CardActions>
                    </Card>
                </Grid>
            </Grid>
            <Dialog open={selectedIndex != null} onClose={onClose}>
                <DialogTitle>Remove a machine</DialogTitle>
                <DialogContent>
                    <DialogContentText>You're about to delete the configuration for the machine {selectedName}!</DialogContentText>
                    <DialogContentText>This action can't be reverted, are you sure?</DialogContentText>
                </DialogContent>
                <DialogActions>
                    <Button onClick={onClose} color="primary">No, Keep it</Button>
                    <Button onClick={onRemove} color="primary">Yes, Delete {selectedName}</Button>
                </DialogActions>
            </Dialog>
        </Box>
    );
};


interface Props {
    projectName: string;
    root: ComponentType;
    machines: ListType;
    entities: ListType;
    onDownload: (entity: string) => void;
    machinesActions: IProjectAction;
    templatesActions: IXsdAction;
}


export default ProjectForm;
