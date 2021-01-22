import React from 'react';

import Accordion from "@material-ui/core/Accordion";
import AccordionSummary from "@material-ui/core/AccordionSummary";
import AccordionDetails from "@material-ui/core/AccordionDetails";
import IconButton from "@material-ui/core/IconButton";
import Paper from '@material-ui/core/Paper';
import Tooltip from "@material-ui/core/Tooltip";
import Typography from "@material-ui/core/Typography";

import {makeStyles, Theme} from '@material-ui/core/styles';
import HelpIcon from "@material-ui/icons/Help";
import ExpandMoreIcon from "@material-ui/icons/ExpandMore";

import {Component as ComponentType} from '../../xsd/model';

import List from './List';
import Parameter from './Parameter';


interface Props {
    component: ComponentType;
    changeModel: () => void;
}


const useStyles = makeStyles((theme: Theme) => ({
    root: {
        width: "98%",
        marginLeft: "1%",
        marginRight: "1%",
    },
    heading: {
        fontSize: theme.typography.pxToRem(15),
        flexBasis: "33.33%",
        flexShrink: 0,
    },
    secondaryHeading: {
        fontSize: theme.typography.pxToRem(15),
        color: theme.palette.text.secondary,
    },
}));


const Component = (props: Props) => {
    const {component, changeModel} = props;
    const classes = useStyles();

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
            {component.children.filter(c => c.isVisible()).map(c => (
                <Accordion key={c.id} defaultExpanded>
                    <AccordionSummary expandIcon={<ExpandMoreIcon />}>
                        <Typography className={classes.heading}>{c.name}</Typography>
                        <Typography className={classes.secondaryHeading}>{c.description}</Typography>
                        {c.description !== "" && (
                            <Tooltip title={c.description} placement="top">
                                <IconButton><HelpIcon /></IconButton>
                            </Tooltip>
                        )}
                    </AccordionSummary>
                    <AccordionDetails>
                        <Component
                            component={c}
                            changeModel={forceUpdate}
                        />
                    </AccordionDetails>
                </Accordion>
            ))}
            {component.lists.filter(l => l.isVisible()).map(l => (
                <Accordion key={l.id} defaultExpanded>
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
        </Paper>
    );
};


export default Component;
