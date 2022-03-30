import React from 'react';
import type {FormikProps} from 'formik';

import Box from '@mui/material/Box';
import Collapse from '@mui/material/Collapse';
import IconButton from '@mui/material/IconButton';
import Table from '@mui/material/Table';
import TableBody from '@mui/material/TableBody';
import TableCell from '@mui/material/TableCell';
import TableContainer from '@mui/material/TableContainer';
import TableHead from '@mui/material/TableHead';
import TableRow from '@mui/material/TableRow';
import Paper from '@mui/material/Paper';

import {styled} from '@mui/material/styles';
import AddIcon from '@mui/icons-material/AddCircleOutline';
import ArrowDownIcon from '@mui/icons-material/KeyboardArrowDown';
import ArrowUpIcon from '@mui/icons-material/KeyboardArrowUp';
import DeleteIcon from '@mui/icons-material/HighlightOff';

import Component from './Component';
import Parameter from './Parameter';

import {useSelector} from '../../redux';
import {noActions} from '../../utils/actions';
import type {IActions} from '../../utils/actions';
import {useListMutators} from '../../utils/hooks';
import {getParameters} from '../../xsd';
import type {Component as ComponentType, List as ListType} from '../../xsd';


const BorderlessTableRow = styled(TableRow, {name: "BorderlessTableRow", slot: "Wrapper"})({
    '& > *': {
        borderBottom: 'unset',
    },
});


const ReducedTableCell = styled(TableCell, {name: "ReducedTableCell", slot: "Wrapper"})({
    paddingTop: 0,
    paddingBottom: 0,
});


const Row: React.FC<RowProps> = (props) => {
    const {component, index, readOnly, isEditable, headers, prefix, form, autosave, actions, onDelete} = props;

    const [open, setOpen] = React.useState<boolean>(false);

    const parameters = getParameters(component, form.values);

    return (
        <React.Fragment>
            <BorderlessTableRow>
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
                {headers.map((id: string, i: number) => {
                    const param = parameters.find(p => p.id === id);
                    const value = param?.value;
                    return (
                        <TableCell key={i+2} align="center">
                            {actions.$.onAction && id === "run" && param ? (
                                <Parameter
                                    parameter={param}
                                    readOnly={readOnly || component.readOnly}
                                    prefix={`${prefix}.elements.${i}.element`}
                                    form={form}
                                    actions={actions['#'][param.id] || noActions}
                                    entity={undefined /* TODO? */}
                                    autosave={autosave}
                                />
                            ) : value}
                        </TableCell>
                    );
                })}
                <TableCell key={headers.length + (actions.$.onAction ? 3 : 2)} align="right">
                    {isEditable && onDelete && (
                        <IconButton size="small" onClick={onDelete}>
                            <DeleteIcon />
                        </IconButton>
                    )}
                </TableCell>
            </BorderlessTableRow>
            <TableRow>
                <ReducedTableCell colSpan={headers.length + 2}>
                    <Collapse in={open} timeout="auto" unmountOnExit>
                        <Box margin={1}>
                            <Component
                                component={component}
                                readOnly={readOnly}
                                prefix={prefix}
                                form={form}
                                actions={actions}
                                autosave={autosave}
                            />
                        </Box>
                    </Collapse>
                </ReducedTableCell>
            </TableRow>
        </React.Fragment>
    );
};


interface RowProps {
    component: ComponentType;
    index: number;
    readOnly?: boolean;
    prefix: string;
    form: FormikProps<ComponentType>;
    isEditable: boolean;
    headers: string[];
    onDelete: false | (() => void);
    actions: IActions;
    autosave: boolean;
}


const List: React.FC<Props> = (props) => {
    const {list, readOnly, prefix, form, actions, autosave} = props;

    const visibility = useSelector((state) => state.form.visibility);
    const [addListItem, removeListItem] = useListMutators(list, actions.$, form, prefix);

    const count = list.elements.length;
    const headers = getParameters(list.pattern, form.values, visibility).map(p => p.id);
    const isEditable = !readOnly && !list.readOnly;
    const canGrow = count < list.maxOccurences;
    const canShrink = count > list.minOccurences;
    const patternActions = actions['#'][list.pattern.id] || noActions;

    return (
        <TableContainer component={Paper}>
            <Table>
                <TableHead>
                    <TableRow>
                        <TableCell key={0} align="left">
                            <div />
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
                            index={i}
                            readOnly={readOnly}
                            prefix={`${prefix}.elements.${i}`}
                            form={form}
                            isEditable={isEditable}
                            headers={headers}
                            onDelete={canShrink && (() => removeListItem(i))}
                            actions={patternActions}
                            autosave={autosave}
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
    form: FormikProps<ComponentType>;
    actions: IActions;
    autosave: boolean;
}


export default List;
