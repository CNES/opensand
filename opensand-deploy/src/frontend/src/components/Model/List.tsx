import React from 'react';

import Box from '@material-ui/core/Box';
import Collapse from '@material-ui/core/Collapse';
import IconButton from '@material-ui/core/IconButton';
import Table from '@material-ui/core/Table';
import TableBody from '@material-ui/core/TableBody';
import TableCell from '@material-ui/core/TableCell';
import TableContainer from '@material-ui/core/TableContainer';
import TableHead from '@material-ui/core/TableHead';
import TableRow from '@material-ui/core/TableRow';
import Paper from '@material-ui/core/Paper';

import { makeStyles } from '@material-ui/core/styles';

import AddIcon from '@material-ui/icons/AddCircleOutline';
import ArrowDownIcon from '@material-ui/icons/KeyboardArrowDown';
import ArrowUpIcon from '@material-ui/icons/KeyboardArrowUp';
import DeleteIcon from '@material-ui/icons/HighlightOff';
import MoreIcon from '@material-ui/icons/MoreHoriz';

import {IActions, noActions} from '../../utils/actions';
import {Component as ComponentType, List as ListType} from '../../xsd/model';

import Component from './Component';


interface Props {
    list: ListType;
    readOnly?: boolean;
    changeModel: () => void;
    actions: IActions;
}


interface RowProps {
    component: ComponentType;
    index: number;
    readOnly?: boolean;
    isEditable: boolean;
    headers: string[];
    changeModel: () => void;
    onDelete: false | (() => void);
    actions: IActions;
}


const useRowStyles = makeStyles({
    root: {
        '& > *': {
            borderBottom: 'unset',
        },
    },
    reduced: {
        paddingTop: 0,
        paddingBottom: 0,
    },
});


const Row = (props: RowProps) => {
    const {component, index, readOnly, isEditable, headers, actions, changeModel, onDelete} = props;
    const classes = useRowStyles();

    const [open, setOpen] = React.useState<boolean>(false);

    const parameters = component.getParameters(false);

    return (
        <React.Fragment>
            <TableRow className={classes.root}>
                <TableCell key={0} align="left">
                    <IconButton size="small" onClick={() => setOpen(!open)}>
                        {open ? <ArrowUpIcon /> : <ArrowDownIcon />}
                    </IconButton>
                </TableCell>
                {actions.$.onAction && (
                    <TableCell key={1} align="center">
                        {actions.$.onAction(index)}
                    </TableCell>
                )}
                {headers.map((id: string, i: number) => (
                    <TableCell key={i+2} align="center">
                        {parameters.find(p => p?.id === id)?.value}
                    </TableCell>
                ))}
                <TableCell key={headers.length + (actions.$.onAction ? 3 : 2)} align="right">
                    {isEditable && onDelete && (
                        <IconButton size="small" onClick={onDelete}>
                            <DeleteIcon />
                        </IconButton>
                    )}
                </TableCell>
            </TableRow>
            <TableRow>
                <TableCell className={classes.reduced} colSpan={headers.length + 2}>
                    <Collapse in={open} timeout="auto" unmountOnExit>
                        <Box margin={1}>
                            <Component
                                component={component}
                                readOnly={readOnly}
                                changeModel={changeModel}
                                actions={actions}
                            />
                        </Box>
                    </Collapse>
                </TableCell>
            </TableRow>
        </React.Fragment>
    );
};


const List = (props: Props) => {
    const {list, readOnly, actions, changeModel} = props;
    const [, setState] = React.useState<object>({});

    const forceUpdate = React.useCallback(() => {
        setState({});
        changeModel();
    }, [changeModel, setState]);

    const addListItem = React.useCallback(() => {
        if (actions.$.onCreate != null) {
            actions.$.onCreate();
        } else {
            list.addItem();
            forceUpdate();
        }
    }, [list, forceUpdate, actions.$]);

    const removeListItem = React.useCallback(() => {
        if (actions.$.onDelete != null) {
            actions.$.onDelete();
        } else {
            list.removeItem();
            forceUpdate();
        }
    }, [list, forceUpdate, actions.$]);

    const count = list.elements.length;
    const headers = list.pattern.getParameters().map(p => p.id);
    const isEditable = !readOnly && !list.readOnly;
    const patternActions = actions['#'][list.pattern.id] || noActions;

    return (
        <TableContainer component={Paper}>
            <Table>
                <TableHead>
                    <TableRow>
                        <TableCell key={0} align="left">
                            <IconButton size="small">
                                <MoreIcon />
                            </IconButton>
                        </TableCell>
                        {patternActions.$.onAction && (
                            <TableCell key={1} align="center">
                                Action
                            </TableCell>
                        )}
                        {headers.map((id: string, i: number) => (
                            <TableCell key={i+2} align="center">
                                {list.pattern.elements.find(p => p.element.id === id)?.element.name}
                            </TableCell>
                        ))}
                        <TableCell key={headers.length + 2} align="right">
                            {isEditable && (
                                <IconButton size="small" onClick={addListItem}>
                                    <AddIcon />
                                </IconButton>
                            )}
                        </TableCell>
                    </TableRow>
                </TableHead>
                <TableBody>
                    {list.elements.map((c: ComponentType, i: number) => (
                        <Row
                            key={i}
                            component={c}
                            index={i}
                            readOnly={readOnly}
                            isEditable={isEditable}
                            headers={headers}
                            changeModel={forceUpdate}
                            onDelete={i === count - 1 && count > list.minOccurences && removeListItem}
                            actions={patternActions}
                        />
                    ))}
                </TableBody>
            </Table>
        </TableContainer>
    );
};


export default List;
