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
            {component.parameters.filter(p => p.isVisible()).map(p => (
                <Parameter
                    key={p.id}
                    parameter={p}
                    changeModel={forceUpdate}
                />
            ))}
            {component.lists.filter(l => l.isVisible()).map(l => (
                <Accordion key={l.id} defaultExpanded={false}>
                    <AccordionSummary expandIcon={<ExpandMoreIcon />}>
                        <Typography className={classes.heading}>{l.name}</Typography>
                        <Typography className={classes.secondaryHeading}>{l.description}</Typography>
                    </AccordionSummary>
                    <AccordionDetails>
                        <List
                            list={l}
                            changeModel={forceUpdate}
                        />
                    </AccordionDetails>
                </Accordion>
            ))}
            {component.children.filter(c => c.isVisible()).map(c => (
                <Accordion key={c.id} defaultExpanded={false}>
                    <AccordionSummary expandIcon={<ExpandMoreIcon />}>
                        <Typography className={classes.heading}>{c.name}</Typography>
                        <Typography className={classes.secondaryHeading}>{c.description}</Typography>
                    </AccordionSummary>
                    <AccordionDetails>
                        <Component
                            component={c}
                            changeModel={forceUpdate}
                        />
                    </AccordionDetails>
                </Accordion>
            ))}
        </Paper>
    );
};


export default Component;
