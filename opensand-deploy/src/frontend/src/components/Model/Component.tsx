import React from 'react';

import Accordion from "@material-ui/core/Accordion";
import AccordionSummary from "@material-ui/core/AccordionSummary";
import AccordionDetails from "@material-ui/core/AccordionDetails";
import Paper from '@material-ui/core/Paper';
import Typography from "@material-ui/core/Typography";

import ExpandMoreIcon from "@material-ui/icons/ExpandMore";

import {IActions, noActions} from '../../utils/actions';
import {componentStyles} from '../../utils/theme';
import {Component as ComponentType, isParameterElement} from '../../xsd/model';

import List from './List';
import Parameter from './Parameter';


interface Props {
    component: ComponentType;
    readOnly?: boolean;
    changeModel: () => void;
    actions: IActions;
}


const findParameterValue = (component: ComponentType, id: string) => {
    const parameter = component.elements.find((e) => e.type === "parameter" && e.element.id === id);
    if (isParameterElement(parameter)) {
        return parameter.element.value;
    }
};


const Component = (props: Props) => {
    const {component, readOnly, actions, changeModel} = props;
    const classes = componentStyles();

    const [, setState] = React.useState<object>({});

    const forceUpdate = React.useCallback(() => {
        setState({});
        changeModel();
    }, [changeModel, setState]);

    if (!component.isVisible()) {
        return null;
    }

    const isReadOnly = readOnly || component.readOnly;
    const entityName = findParameterValue(component, "entity_name");
    const entityType = findParameterValue(component, "entity_type");
    const entity = entityName != null && entityType != null ? {name: entityName, type: entityType} : undefined;

    return (
        <Paper elevation={0} className={classes.root}>
            {component.elements.filter(e => e.element.isVisible()).map(e => {
                const elementActions = actions['#'][e.element.id] || noActions;
                switch (e.type) {
                    case "parameter":
                        return (
                            <Parameter
                                key={e.element.id}
                                parameter={e.element}
                                readOnly={isReadOnly}
                                changeModel={forceUpdate}
                                actions={elementActions}
                                entity={entity}
                            />
                        );
                    case "list":
                        return (
                            <Accordion key={e.element.id} defaultExpanded={false}>
                                <AccordionSummary expandIcon={<ExpandMoreIcon />}>
                                    <Typography className={classes.heading}>
                                        {e.element.name}
                                    </Typography>
                                    <Typography className={classes.secondaryHeading}>
                                        {e.element.description}
                                    </Typography>
                                </AccordionSummary>
                                <AccordionDetails>
                                    <List
                                        list={e.element}
                                        readOnly={isReadOnly}
                                        changeModel={forceUpdate}
                                        actions={elementActions}
                                    />
                                </AccordionDetails>
                            </Accordion>
                        );
                    case "component":
                        return (
                            <Accordion key={e.element.id} defaultExpanded={false}>
                                <AccordionSummary expandIcon={<ExpandMoreIcon />}>
                                    <Typography className={classes.heading}>
                                        {e.element.name}
                                    </Typography>
                                    <Typography className={classes.secondaryHeading}>
                                        {e.element.description}
                                    </Typography>
                                </AccordionSummary>
                                <AccordionDetails>
                                    <Component
                                        component={e.element}
                                        readOnly={isReadOnly}
                                        changeModel={forceUpdate}
                                        actions={elementActions}
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


export default Component;
