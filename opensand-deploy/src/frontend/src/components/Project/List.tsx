import React from 'react';

import IconButton from '@material-ui/core/IconButton';
import Paper from '@material-ui/core/Paper';
import Table from '@material-ui/core/Table';
import TableBody from '@material-ui/core/TableBody';
import TableCell from '@material-ui/core/TableCell';
import TableContainer from '@material-ui/core/TableContainer';
import TableHead from '@material-ui/core/TableHead';
import TableRow from '@material-ui/core/TableRow';
import Tooltip from '@material-ui/core/Tooltip';

import AddIcon from '@material-ui/icons/AddCircleOutline';
import DeleteIcon from '@material-ui/icons/HighlightOff';
import DownloadIcon from '@material-ui/icons/GetApp';

import {ITemplatesContent} from '../../api';
import {Component, List, Parameter, Enum} from '../../xsd/model';

import Parameters from './Parameter';
import SingleFieldDialog from '../common/SingleFieldDialog';


interface Props {
    list: List;
    templates: ITemplatesContent;
    forceUpdate: () => void;
    entityTypes: EntityTypes;
    onSelect: (entity: string) => void;
    onEdit: (entity: string | null, model: string, xsd: string, xml?: string) => void;
    onDelete: (entity: string | null, model: string) => void;
    onDownload: (entity: string | null) => void;
}


interface EntityTypes {
    [entityName: string]: string | undefined;
}


const ProjectList = (props: Props) => {
    const {list, templates, forceUpdate} = props;
    const {enums} = list.model.environment;
    const [open, setOpen] = React.useState<boolean>(false);

    const handleOpen = React.useCallback(() => {
        setOpen(true);
    }, [setOpen]);

    const handleClose = React.useCallback(() => {
        setOpen(false);
    }, [setOpen]);

    const addListItem = React.useCallback((name: string) => {
        list.addItem();
        const param = list.elements[list.elements.length - 1].parameters.find((p: Parameter) => p.id === "name");
        if (param) {param.value = name;}
        setOpen(false);
        forceUpdate();
    }, [list, forceUpdate, setOpen]);

    const removeListItem = React.useCallback((index: number) => {
        list.elements.splice(index, 1);
        forceUpdate();
    }, [list, forceUpdate]);

    const headers = list.pattern.parameters.map((p: Parameter) => p.id);

    return (
        <TableContainer component={Paper}>
            <Table>
                <TableHead>
                    <TableRow>
                        <TableCell key={0} align="left">
                            <Tooltip placement="top" title="Download configuration files of the project">
                                <IconButton size="small" onClick={props.onDownload.bind(this, null)}>
                                    <DownloadIcon />
                                </IconButton>
                            </Tooltip>
                        </TableCell>
                        {headers.map((id: string, i: number) => (
                            <TableCell key={i+1} align="center">
                                {list.pattern.parameters.find(p => p.id === id)?.name}
                            </TableCell>
                        ))}
                        <TableCell key={headers.length + 1} align="right">
                            <Tooltip placement="top" title="Add a new entity to the project">
                                <IconButton size="small" onClick={handleOpen}>
                                    <AddIcon />
                                </IconButton>
                            </Tooltip>
                        </TableCell>
                    </TableRow>
                </TableHead>
                <TableBody>
                    {list.elements.map((c: Component, i: number) => {
                        const entity = c.parameters.find((p: Parameter) => p.id === "name")?.value;
                        const entityType = props.entityTypes[entity || ""];
                        const onEdit = props.onEdit.bind(this, entity || null);
                        const onDelete = props.onDelete.bind(this, entity || null);
                        const onDownload = props.onDownload.bind(this, entity || null);
                        const onSelect = props.onSelect.bind(this, entity || "");
                        return (
                            <TableRow key={i}>
                                <TableCell key={0} align="left">
                                    <Tooltip placement="top" title="Download configuration files of this entity">
                                        <IconButton size="small" onClick={onDownload}>
                                            <DownloadIcon />
                                        </IconButton>
                                    </Tooltip>
                                </TableCell>
                                {headers.map((id: string, i: number) => {
                                    const param = c.parameters.find((p: Parameter) => p.id === id);
                                    if (param == null) {
                                        return <TableCell key={i+1} align="center" />;
                                    }
                                    return (
                                        <TableCell key={i+1} align="center">
                                            <Parameters
                                                key={i+1}
                                                parameter={param}
                                                templates={templates}
                                                entityType={entityType}
                                                onEdit={onEdit}
                                                onDelete={onDelete}
                                                onSelect={param.id === "infrastructure" ? onSelect : undefined}
                                                enumeration={enums.find((e: Enum) => e.id === param.type)}
                                            />
                                        </TableCell>
                                    );
                                })}
                                <TableCell key={headers.length + 1} align="right">
                                    <Tooltip placement="top" title="Remove this entity">
                                        <IconButton size="small" onClick={() => removeListItem(i)}>
                                            <DeleteIcon />
                                        </IconButton>
                                    </Tooltip>
                                </TableCell>
                            </TableRow>
                        );
                    })}
                </TableBody>
            </Table>
            <SingleFieldDialog
                open={open}
                title="New Entity"
                description="Please enter the name of your new entity."
                fieldLabel="Entity Name"
                onValidate={addListItem}
                onClose={handleClose}
            />
        </TableContainer>
    );
};


export default ProjectList;
