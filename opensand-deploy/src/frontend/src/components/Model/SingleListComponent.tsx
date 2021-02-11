import React from 'react';

import Collapse from '@material-ui/core/Collapse';
import IconButton from '@material-ui/core/IconButton';
import List from '@material-ui/core/List';
import ListItem from '@material-ui/core/ListItem';
import ListItemIcon from '@material-ui/core/ListItemIcon';
import ListItemSecondaryAction from '@material-ui/core/ListItemSecondaryAction';
import ListItemText from '@material-ui/core/ListItemText';

import {makeStyles, Theme} from '@material-ui/core/styles';

import AddIcon from '@material-ui/icons/AddCircleOutline';
import DeleteIcon from '@material-ui/icons/HighlightOff';

import {List as ListType, Component as ComponentType} from '../../xsd/model';

import Component from './Component';


interface Props {
    list: ListType;
    changeModel: () => void;
}


const useStyles = makeStyles((theme: Theme) => ({
    root: {
        display: "flex",
    },

    leftPanel: {
        color: theme.palette.type === "dark" ? theme.palette.common.black : theme.palette.text.primary,
        flexGrow: 1,
        '& .MuiListItem-root': {
            backgroundColor: "#FFFACD",
            textTransform: "capitalize",
            '&.Mui-selected': {
                backgroundColor: "#FFE1AC",
            },
        },
    },

    rightPanel: {
        flexGrow: 4,
    },
}));


const SingleListComponent = (props: Props) => {
    const {list, changeModel} = props;
    const classes = useStyles();

    const [open, setOpen] = React.useState<number>(0);
    const [, setState] = React.useState<object>({});

    const forceUpdate = React.useCallback(() => {
        setState({});
        changeModel();
    }, [changeModel, setState]);

    const addListItem = React.useCallback(() => {
        list.addItem();
        forceUpdate();
    }, [list, forceUpdate]);

    const removeListItem = React.useCallback(() => {
        list.removeItem();
        forceUpdate();
    }, [list, forceUpdate]);

    return (
        <div className={classes.root}>
            <List className={classes.leftPanel}>
                <ListItem key={0} button selected onClick={addListItem}>
                    <ListItemIcon><AddIcon /></ListItemIcon>
                    <ListItemText primary={`Add New ${list.pattern.name}`} />
                </ListItem>
                {list.elements.map((c: ComponentType, i: number) => (
                    <ListItem key={i+1} button selected={i === open} onClick={() => setOpen(i)}>
                        <ListItemText primary={c.name} />
                        {i + 1=== list.elements.length && (
                            <ListItemSecondaryAction>
                                <IconButton edge="end" onClick={removeListItem} color="inherit">
                                    <DeleteIcon />
                                </IconButton>
                            </ListItemSecondaryAction>
                        )}
                    </ListItem>
                ))}
            </List>
            <div className={classes.rightPanel}>
                {list.elements.map((c: ComponentType, i: number) => (
                    <Collapse in={open === i} timeout="auto" unmountOnExit>
                        <Component component={c} changeModel={changeModel} />
                    </Collapse>
                ))}
            </div>
        </div>
    );
};


export default SingleListComponent;
