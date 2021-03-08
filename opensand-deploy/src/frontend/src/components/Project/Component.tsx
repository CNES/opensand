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
    onSelect: (entity: string, setEntityType: (entityType?: string) => void) => void;
    onEdit: (entity: string | null, model: string, xsd: string, xml?: string) => void;
    onDelete: (entity: string | null, model: string) => void;
    onDownload: (entity: string | null) => void;
}


const ProjectComponent = (props: Props) => {
    const {component, templates, forceUpdate, onSelect, onDownload} = props;
    const {enums} = component.model.environment;
    const classes = componentStyles();
    const onEdit = props.onEdit.bind(this, null);
    const onDelete = props.onDelete.bind(this, null);

    return (
        <Paper elevation={0} className={classes.root}>
            {component.elements.filter(e => e.element.isVisible()).map(e => {
                const id = e.element.id;
                switch (e.type) {
                    case "parameter":
                        const type = e.element.type;
                        return (
                            <Parameters
                                key={id}
                                parameter={e.element}
                                templates={templates}
                                onEdit={onEdit}
                                onDelete={onDelete}
                                enumeration={enums.find((enumeration: Enum) => enumeration.id === type)}
                            />
                        );
                    case "list":
                        return (
                            <Accordion key={id} defaultExpanded>
                                <AccordionSummary expandIcon={<ExpandMoreIcon />}>
                                    <Typography className={classes.heading}>
                                        {e.element.name}
                                    </Typography>
                                    <Typography className={classes.secondaryHeading}>
                                        {e.element.description}
                                    </Typography>
                                </AccordionSummary>
                                <AccordionDetails>
                                    <Lists
                                        list={e.element}
                                        templates={templates}
                                        forceUpdate={forceUpdate}
                                        onSelect={onSelect}
                                        onEdit={props.onEdit}
                                        onDelete={props.onDelete}
                                        onDownload={onDownload}
                                    />
                                </AccordionDetails>
                            </Accordion>
                        );
                    case "component":
                        return (
                            <Accordion key={id} defaultExpanded>
                                <AccordionSummary expandIcon={<ExpandMoreIcon />}>
                                    <Typography className={classes.heading}>
                                        {e.element.name}
                                    </Typography>
                                    <Typography className={classes.secondaryHeading}>
                                        {e.element.description}
                                    </Typography>
                                </AccordionSummary>
                                <AccordionDetails>
                                    <ProjectComponent
                                        component={e.element}
                                        templates={templates}
                                        onSelect={onSelect}
                                        onEdit={props.onEdit}
                                        onDelete={props.onDelete}
                                        onDownload={onDownload}
                                        forceUpdate={forceUpdate}
                                    />
                                </AccordionDetails>
                            </Accordion>
                        );
                    default:
                        return <div />;
                }
            })}
        </Paper>
    );
};


export default ProjectComponent;
