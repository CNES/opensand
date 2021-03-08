import React from 'react';

import Accordion from "@material-ui/core/Accordion";
import AccordionSummary from "@material-ui/core/AccordionSummary";
import AccordionDetails from "@material-ui/core/AccordionDetails";
import Paper from '@material-ui/core/Paper';
import Typography from "@material-ui/core/Typography";

import ExpandMoreIcon from "@material-ui/icons/ExpandMore";

import {componentStyles} from '../../utils/theme';
import {Component as ComponentType} from '../../xsd/model';

import List from './List';
import Parameter from './Parameter';


interface Props {
    component: ComponentType;
    changeModel: () => void;
}


const Component = (props: Props) => {
    const {component, changeModel} = props;
    const classes = componentStyles();

    const [, setState] = React.useState<object>({});

    const forceUpdate = React.useCallback(() => {
        setState({});
        changeModel();
    }, [changeModel, setState]);

    if (!component.isVisible()) {
        return null;
    }

    return (
        <Paper elevation={0} className={classes.root}>
            {component.elements.filter(e => e.element.isVisible()).map(e => {
                switch (e.type) {
                    case "parameter":
                        return (
                            <Parameter
                                key={e.element.id}
                                parameter={e.element}
                                changeModel={forceUpdate}
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
                                        changeModel={forceUpdate}
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
                                        changeModel={forceUpdate}
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
