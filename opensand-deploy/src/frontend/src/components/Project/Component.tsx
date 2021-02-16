import React from 'react';

import Accordion from "@material-ui/core/Accordion";
import AccordionSummary from "@material-ui/core/AccordionSummary";
import AccordionDetails from "@material-ui/core/AccordionDetails";
import Paper from '@material-ui/core/Paper';
import Typography from "@material-ui/core/Typography";

import ExpandMoreIcon from "@material-ui/icons/ExpandMore";

import {ITemplatesContent} from '../../api';
import {componentStyles} from '../../utils/theme';
import {Component, Enum} from '../../xsd/model';

import Parameters from './Parameter';
import Lists from './List';


interface Props {
    component: Component;
    templates: ITemplatesContent;
    forceUpdate: () => void;
    entityTypes: EntityTypes;
    onSelect: (entity: string) => void;
    onEdit: (entity: string | null, model: string, xsd: string, xml?: string) => void;
    onDelete: (entity: string | null, model: string) => void;
    onDownload: (entity: string | null) => void;
}


interface EntityTypes {
    [entityName: string]: string | undefined;
}


const ProjectComponent = (props: Props) => {
    const {component, templates, forceUpdate, entityTypes, onSelect, onDownload} = props;
    const {enums} = component.model.environment;
    const classes = componentStyles();
    const onEdit = props.onEdit.bind(this, null);
    const onDelete = props.onDelete.bind(this, null);

    return (
        <Paper elevation={0} className={classes.root}>
            {component.parameters.filter(p => p.isVisible()).map(p => (
                <Parameters
                    key={p.id}
                    parameter={p}
                    templates={templates}
                    onEdit={onEdit}
                    onDelete={onDelete}
                    enumeration={enums.find((e: Enum) => e.id === p.type)}
                />
            ))}
            {component.lists.filter(l => l.isVisible()).map(l => (
                <Accordion key={l.id} defaultExpanded>
                    <AccordionSummary expandIcon={<ExpandMoreIcon />}>
                        <Typography className={classes.heading}>{l.name}</Typography>
                        <Typography className={classes.secondaryHeading}>{l.description}</Typography>
                    </AccordionSummary>
                    <AccordionDetails>
                        <Lists
                            list={l}
                            templates={templates}
                            forceUpdate={forceUpdate}
                            entityTypes={entityTypes}
                            onSelect={onSelect}
                            onEdit={props.onEdit}
                            onDelete={props.onDelete}
                            onDownload={onDownload}
                        />
                    </AccordionDetails>
                </Accordion>
            ))}
            {component.children.filter(c => c.isVisible()).map(c => (
                <Accordion key={c.id} defaultExpanded>
                    <AccordionSummary expandIcon={<ExpandMoreIcon />}>
                        <Typography className={classes.heading}>{c.name}</Typography>
                        <Typography className={classes.secondaryHeading}>{c.description}</Typography>
                    </AccordionSummary>
                    <AccordionDetails>
                        <ProjectComponent
                            component={c}
                            templates={templates}
                            entityTypes={entityTypes}
                            onSelect={onSelect}
                            onEdit={props.onEdit}
                            onDelete={props.onDelete}
                            onDownload={onDownload}
                            forceUpdate={forceUpdate}
                        />
                    </AccordionDetails>
                </Accordion>
            ))}
        </Paper>
    );
};


export default ProjectComponent;