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
// import Typography from '@material-ui/core/Typography';
import Paper from '@material-ui/core/Paper';

import { makeStyles } from '@material-ui/core/styles';

import AddIcon from '@material-ui/icons/AddCircleOutline';
import ArrowDownIcon from '@material-ui/icons/KeyboardArrowDown';
import ArrowUpIcon from '@material-ui/icons/KeyboardArrowUp';
import DeleteIcon from '@material-ui/icons/HighlightOff';
import MoreIcon from '@material-ui/icons/MoreHoriz';

import {Component as ComponentType, List as ListType} from '../../xsd/model';

import Component from './Component';


interface Props {
    list: ListType;
    changeModel: () => void;
}


interface RowProps {
    component: ComponentType;
    headers: string[];
    changeModel: () => void;
    onDelete: false | (() => void);
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
    const {component, headers, changeModel, onDelete} = props;
    const [open, setOpen] = React.useState<boolean>(true);
    const classes = useRowStyles();

    return (
        <React.Fragment>
            <TableRow className={classes.root}>
                <TableCell key={0}>
                    <IconButton size="small" onClick={() => setOpen(!open)}>
                        {open ? <ArrowUpIcon /> : <ArrowDownIcon />}
                    </IconButton>
                </TableCell>
                {headers.map((id: string, i: number) => (
                    <TableCell key={i+1}>
                        {component.parameters.find(p => p.id === id)?.value}
                    </TableCell>
                ))}
                <TableCell key={headers.length + 1}>
                    {onDelete &&
                    <IconButton size="small" onClick={onDelete}>
                        <DeleteIcon />
                    </IconButton>}
                </TableCell>
            </TableRow>
            <TableRow>
                <TableCell className={classes.reduced} colSpan={headers.length + 2}>
                    <Collapse in={open} timeout="auto" unmountOnExit>
                        <Box margin={1}>
                            <Component component={component} changeModel={changeModel} />
                        </Box>
                    </Collapse>
                </TableCell>
            </TableRow>
        </React.Fragment>
    );
};


const List = (props: Props) => {
    const {list, changeModel} = props;
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

    const headers = list.pattern.parameters.filter(p => p.isVisible()).map(p => p.id);

    return (
        <TableContainer component={Paper}>
            <Table>
                <TableHead>
                    <TableRow>
                        <TableCell key={0}>
                            <IconButton size="small">
                                <MoreIcon />
                            </IconButton>
                        </TableCell>
                        {headers.map((id: string, i: number) => (
                            <TableCell key={i+1}>
                                {list.pattern.parameters.find(p => p.id === id)?.name}
                            </TableCell>
                        ))}
                        <TableCell key={headers.length + 1}>
                            <IconButton size="small" onClick={addListItem}>
                                <AddIcon />
                            </IconButton>
                        </TableCell>
                    </TableRow>
                </TableHead>
                <TableBody>
                    {list.elements.map((c: ComponentType, i: number) => (
                        <Row
                            key={i}
                            component={c}
                            headers={headers}
                            changeModel={forceUpdate}
                            onDelete={i === list.elements.length - 1 && removeListItem}
                        />
                    ))}
                </TableBody>
            </Table>
        </TableContainer>
    );
};


export default List;
