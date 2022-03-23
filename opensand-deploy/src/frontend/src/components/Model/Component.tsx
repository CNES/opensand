import React from 'react';
import type {FormikProps} from 'formik';

import Accordion from "@mui/material/Accordion";
import AccordionSummary from "@mui/material/AccordionSummary";
import AccordionDetails from "@mui/material/AccordionDetails";
import Paper from '@mui/material/Paper';
import Typography from "@mui/material/Typography";

import {styled} from '@mui/material/styles';
import ExpandMoreIcon from "@mui/icons-material/ExpandMore";

import List from './List';
import Parameter from './Parameter';

import {useSelector} from '../../redux';
import {IActions, noActions} from '../../utils/actions';
import {isParameterElement, isVisible} from '../../xsd';
import type {Element, Component as ComponentType} from '../../xsd';


const LargePaper = styled(Paper, {name: "LargePaper", slot: "Wrapper"})(({ theme }) => ({
    width: "98%",
    marginLeft: "1%",
    marginRight: "1%",
    marginTop: theme.spacing(1),
}));


const Heading = styled(Typography, {name: "Heading", slot: "Wrapper"})(({ theme }) => ({
    fontSize: theme.typography.pxToRem(15),
    flexBasis: "33.33%",
    flexShrink: 0,
}));


const SecondaryHeading = styled(Typography, {name: "SecondaryHeading", slot: "Wrapper"})(({ theme }) => ({
    fontSize: theme.typography.pxToRem(15),
    color: theme.palette.text.secondary,
}));


const findParameterValue = (component: ComponentType, id: string) => {
    const parameter = component.elements.find((e) => isParameterElement(e) && e.element.id === id);
    if (isParameterElement(parameter)) {
        return parameter.element.value;
    }
};


const Component: React.FC<Props> = (props) => {
    const {component, readOnly, prefix, form, actions, autosave} = props;

    const visibility = useSelector((state) => state.form.visibility);

    if (!isVisible(component, visibility, form.values)) {
        return null;
    }

    const isReadOnly = readOnly || component.readOnly;
    const entityName = findParameterValue(component, "entity_name");
    const entityType = findParameterValue(component, "entity_type");
    const entity = entityName != null && entityType != null ? {name: entityName, type: entityType} : undefined;

    const visibleComponents = component.elements.map(
        (e, i): [number, Element] => [i, e]
    ).filter(
        ([i, e]: [number, Element]): boolean => isVisible(e.element, visibility, form.values)
    );

    return (
        <LargePaper elevation={0}>
            {visibleComponents.map(([i, e]: [number, Element]) => {
                const childPrefix = `${prefix}.elements.${i}.element`;
                const elementActions = actions['#'][e.element.id] || noActions;
                switch (e.type) {
                    case "parameter":
                        return (
                            <Parameter
                                key={e.element.id}
                                parameter={e.element}
                                readOnly={isReadOnly}
                                prefix={childPrefix}
                                form={form}
                                actions={elementActions}
                                entity={entity}
                                autosave={autosave}
                            />
                        );
                    case "list":
                        return (
                            <Accordion key={e.element.id} defaultExpanded={false} TransitionProps={{unmountOnExit: true}}>
                                <AccordionSummary expandIcon={<ExpandMoreIcon />}>
                                    <Heading>
                                        {e.element.name}
                                    </Heading>
                                    <SecondaryHeading>
                                        {e.element.description}
                                    </SecondaryHeading>
                                </AccordionSummary>
                                <AccordionDetails>
                                    <List
                                        list={e.element}
                                        readOnly={isReadOnly}
                                        prefix={childPrefix}
                                        form={form}
                                        actions={elementActions}
                                        autosave={autosave}
                                    />
                                </AccordionDetails>
                            </Accordion>
                        );
                    case "component":
                        return (
                            <Accordion key={e.element.id} defaultExpanded={false} TransitionProps={{unmountOnExit: true}}>
                                <AccordionSummary expandIcon={<ExpandMoreIcon />}>
                                    <Heading>
                                        {e.element.name}
                                    </Heading>
                                    <SecondaryHeading>
                                        {e.element.description}
                                    </SecondaryHeading>
                                </AccordionSummary>
                                <AccordionDetails>
                                    <Component
                                        component={e.element}
                                        readOnly={isReadOnly}
                                        prefix={childPrefix}
                                        form={form}
                                        actions={elementActions}
                                        autosave={autosave}
                                    />
                                </AccordionDetails>
                            </Accordion>
                        );
                    default:
                        return <div />;
                }
            })}
        </LargePaper>
    );
};


interface Props {
    component: ComponentType;
    readOnly?: boolean;
    prefix: string;
    form: FormikProps<ComponentType>;
    actions: IActions;
    autosave: boolean;
}


export default Component;
