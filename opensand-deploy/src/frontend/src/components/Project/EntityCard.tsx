import React from 'react';

import Avatar from '@mui/material/Avatar';
import Button from '@mui/material/Button';
import Card from '@mui/material/Card';
import CardActions from '@mui/material/CardActions';
import CardContent from '@mui/material/CardContent';
import CardHeader from '@mui/material/CardHeader';
import Collapse from '@mui/material/Collapse';
import Divider from '@mui/material/Divider';

import Delete from '@mui/icons-material/DeleteForever';
import DownloadIcon from '@mui/icons-material/GetApp';
import Gateway from '@mui/icons-material/CellTower';
import HideIcon from '@mui/icons-material/ExpandLess';
import Satellite from '@mui/icons-material/SatelliteAlt';
import ShowIcon from '@mui/icons-material/ExpandMore';
import Terminal from '@mui/icons-material/Devices';

import Component from '../Model/Component';
import {XsdParameter} from '../Model/Parameter';
import EntityAction from './EntityAction';

import type {IXsdAction} from '../../utils/actions';
import {isParameterElement} from '../../xsd';
import type {Component as ComponentType} from '../../xsd';


const EntityCard: React.FC<Props> = (props) => {
    const {project, index, machine, entity, onDownload, removeMachine , templatesActions} = props;
    const [open, setOpen] = React.useState<boolean>(false);

    const toggleOpen = React.useCallback(() => {
        setOpen((o) => !o);
    }, []);

    const name = machine.elements.find((e) => e.element.id === "entity_name");
    const type = machine.elements.find((e) => e.element.id === "entity_type");

    if (!isParameterElement(type) || !isParameterElement(name)) {
        // Project.tsx already checked that entity exists and has the same entity_name and
        // entity_type parameter values; no need to repeat that check here. We only please
        // typescript with this check but it should always be true.
        return null;
    }
    const entityTag = {name: name.element.value, type: type.element.value};
    const icon = entityTag.type === 'Satellite' ? <Satellite /> : entityTag.type === 'Terminal' ? <Terminal /> : <Gateway />;

    return (
        <Card variant="outlined">
            <CardHeader
                title={entityTag.name}
                subheader={entityTag.type}
                avatar={<Avatar>{icon}</Avatar>}
            />
            <CardContent>
                {entity.elements.map((parameter, idx: number) => {
                    if (isParameterElement(parameter) && parameter.element.id.endsWith('__template')) {
                        const name = parameter.element.id;
                        return (
                            <XsdParameter
                                key={name}
                                parameter={parameter.element}
                                readOnly={false}
                                name={`elements.1.element.elements.2.element.elements.${index}.elements.${idx}.element.value`}
                                actions={templatesActions}
                                entity={entityTag}
                            />
                        );
                    }
                    return null;
                })}
                <Divider textAlign="left" variant="middle" onClick={toggleOpen} sx={{position: 'relative', cursor: 'pointer'}}>
                    Machine
                    {open && <HideIcon sx={{position: 'absolute', top: -2, left: -10}}/>}
                    {!open && <ShowIcon sx={{position: 'absolute', top: -2, left: -10}}/>}
                </Divider>
                <Collapse in={open}>
                    <Component
                        component={machine}
                        readOnly={false}
                        prefix={`elements.0.element.elements.1.element.elements.${index}`}
                    />
                </Collapse>
            </CardContent>
            <CardActions>
                <Button
                    variant="outlined"
                    color="error"
                    startIcon={<Delete />}
                    onClick={() => removeMachine(index)}
                >
                    Remove
                </Button>
                <Button
                    variant="outlined"
                    startIcon={<DownloadIcon />}
                    onClick={() => onDownload(entityTag.name)}
                >
                    Download
                </Button>
                <EntityAction project={project} entity={machine} />
            </CardActions>
        </Card>
    );
};


interface Props {
    project: string;
    index: number;
    entity: ComponentType;
    machine: ComponentType;
    onDownload: (entity: string) => void;
    removeMachine: (index: number) => void;
    templatesActions: IXsdAction;
}


export default EntityCard;
