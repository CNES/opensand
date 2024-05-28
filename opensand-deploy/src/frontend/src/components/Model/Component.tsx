import React from 'react';
import {useFormikContext} from 'formik';

import Accordion from "@mui/material/Accordion";
import AccordionSummary from "@mui/material/AccordionSummary";
import AccordionDetails from "@mui/material/AccordionDetails";
import Box from '@mui/material/Box';
import Typography from "@mui/material/Typography";

import {styled} from '@mui/material/styles';
import ExpandMoreIcon from "@mui/icons-material/ExpandMore";

import List from './List';
import Parameter from './Parameter';

import {useSelector} from '../../redux';
import {isVisible} from '../../xsd';
import type {Element, Component as ComponentType} from '../../xsd';


const Heading = styled(Typography, {name: "Heading", slot: "Wrapper"})(({ theme }) => ({
    fontSize: theme.typography.pxToRem(15),
    flexBasis: "33.33%",
    flexShrink: 0,
}));


const SecondaryHeading = styled(Typography, {name: "SecondaryHeading", slot: "Wrapper"})(({ theme }) => ({
    fontSize: theme.typography.pxToRem(15),
    color: theme.palette.text.secondary,
}));


const Component: React.FC<Props> = (props) => {
    const {component, readOnly, prefix, padding=2} = props;
    const {values} = useFormikContext<ComponentType>();

    const visibility = useSelector((state) => state.form.visibility);

    if (!isVisible(component, visibility, values)) {
        return null;
    }

    const isReadOnly = readOnly || component.readOnly;

    const visibleComponents = component.elements.map(
        (e: Element, i: number): [number, Element] => [i, e]
    ).filter(
        ([i, e]: [number, Element]): boolean => isVisible(e.element, visibility, values)
    );

    return (
        <Box p={padding} pb={2}>
            {visibleComponents.map(([i, e]: [number, Element]) => {
                const childPrefix = `${prefix}.elements.${i}.element`;
                switch (e.type) {
                    case "parameter":
                        return (
                            <Parameter
                                key={e.element.id}
                                parameter={e.element}
                                readOnly={isReadOnly}
                                prefix={childPrefix}
                            />
                        );
                    case "list":
                        return (
                            <Accordion key={e.element.id} defaultExpanded TransitionProps={{unmountOnExit: true}}>
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
                                    />
                                </AccordionDetails>
                            </Accordion>
                        );
                    case "component":
                        return (
                            <Accordion key={e.element.id} defaultExpanded TransitionProps={{unmountOnExit: true}}>
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
                                    />
                                </AccordionDetails>
                            </Accordion>
                        );
                    default:
                        return null;
                }
            })}
        </Box>
    );
};


interface Props {
    component: ComponentType;
    readOnly?: boolean;
    prefix: string;
    padding?: number;
}


export default Component;
