import React from 'react';
import {useFormikContext} from 'formik';

import Collapse from '@mui/material/Collapse';
import IconButton from '@mui/material/IconButton';
import Table from '@mui/material/Table';
import TableBody from '@mui/material/TableBody';
import TableCell from '@mui/material/TableCell';
import TableContainer from '@mui/material/TableContainer';
import TableHead from '@mui/material/TableHead';
import TableRow from '@mui/material/TableRow';
import Paper from '@mui/material/Paper';

import AddIcon from '@mui/icons-material/AddCircleOutline';
import ArrowDownIcon from '@mui/icons-material/KeyboardArrowDown';
import ArrowUpIcon from '@mui/icons-material/KeyboardArrowUp';
import DeleteIcon from '@mui/icons-material/HighlightOff';

import Component from './Component';

import {useSelector} from '../../redux';
import {useListMutators} from '../../utils/hooks';
import {getParameters} from '../../xsd';
import type {Component as ComponentType, List as ListType} from '../../xsd';


const Row: React.FC<RowProps> = (props) => {
    const {component, readOnly, isEditable, headers, prefix, onDelete} = props;
    const form = useFormikContext<ComponentType>();

    const [open, setOpen] = React.useState<boolean>(false);

    const parameters = getParameters(component, form.values);

    return (
        <React.Fragment>
            <TableRow>
                <TableCell key="show" align="left" sx={open ? {} : {borderBottom: "unset"}}>
                    <IconButton size="small" onClick={() => setOpen(!open)}>
                        {open ? <ArrowUpIcon /> : <ArrowDownIcon />}
                    </IconButton>
                </TableCell>
                {headers.map((id: string, i: number) => {
                    const param = parameters.find(p => p.id === id);
                    const value = param?.value;
                    return (
                        <TableCell key={i} align="center" sx={open ? {} : {borderBottom: "unset"}}>
                            {value}
                        </TableCell>
                    );
                })}
                <TableCell key="delete" align="right" sx={open ? {} : {borderBottom: "unset"}}>
                    {isEditable && onDelete && (
                        <IconButton size="small" onClick={onDelete}>
                            <DeleteIcon />
                        </IconButton>
                    )}
                </TableCell>
            </TableRow>
            <TableRow>
                <TableCell colSpan={headers.length + 2} sx={{padding: 0}}>
                    <Collapse in={open} timeout="auto" unmountOnExit>
                        <Component
                            component={component}
                            readOnly={readOnly}
                            prefix={prefix}
                        />
                    </Collapse>
                </TableCell>
            </TableRow>
        </React.Fragment>
    );
};


interface RowProps {
    component: ComponentType;
    readOnly?: boolean;
    prefix: string;
    isEditable: boolean;
    headers: string[];
    onDelete: false | (() => void);
}


const List: React.FC<Props> = (props) => {
    const {list, readOnly, prefix} = props;
    const form = useFormikContext<ComponentType>();

    const visibility = useSelector((state) => state.form.visibility);
    const [addListItem, removeListItem] = useListMutators(list, prefix);

    const count = list.elements.length;
    const headers = getParameters(list.pattern, form.values, visibility).map(p => p.id);
    const isEditable = !readOnly && !list.readOnly;
    const canGrow = count < list.maxOccurences;
    const canShrink = count > list.minOccurences;

    return (
        <TableContainer component={Paper}>
            <Table>
                <TableHead>
                    <TableRow>
                        <TableCell key="show" align="left">
                            <div />
                        </TableCell>
                        {headers.map((id: string, i: number) => (
                            <TableCell key={i} align="center">
                                {list.pattern.elements.find(p => p.element.id === id)?.element.name}
                            </TableCell>
                        ))}
                        <TableCell key="add" align="right">
                            {isEditable && canGrow && (
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
                            readOnly={readOnly}
                            prefix={`${prefix}.elements.${i}`}
                            isEditable={isEditable}
                            headers={headers}
                            onDelete={canShrink && (() => removeListItem(i))}
                        />
                    ))}
                </TableBody>
            </Table>
        </TableContainer>
    );
};


interface Props {
    list: ListType;
    readOnly?: boolean;
    prefix: string;
}


export default List;
