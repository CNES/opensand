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
    onSelect: (entity: string, setEntityType: (entityType?: string) => void) => void;
    onEdit: (entity: string | null, model: string, xsd: string, xml?: string) => void;
    onDelete: (entity: string | null, model: string) => void;
    onDownload: (entity: string | null) => void;
}


interface EntityItemProps {
    entity?: string;
    onSelect: (entity: string, setEntityType: (entityType?: string) => void) => void;
    onEdit: (entity: string | null, model: string, xsd: string, xml?: string) => void;
    onDelete: (entity: string | null, model: string) => void;
    onDownload: (entity: string | null) => void;
    onRemove: () => void;
    headers: string[];
    parameters: Parameter[];
    templates: ITemplatesContent;
    enumerations: Enum[];
}


const ProjectListEntityItem = (props: EntityItemProps) => {
    const {entity, onSelect, onEdit, onDelete, onDownload, onRemove, headers, parameters, templates, enumerations} = props;
    const [entityType, setEntityType] = React.useState<string | undefined>(undefined);

    const handleSelect = React.useCallback(() => {
        if (entity != null) {
            onSelect(entity, setEntityType);
        }
    }, [entity, onSelect]);

    const handleEdit = React.useCallback((model: string, xsd: string, xml?: string) => {
        onEdit(entity || null, model, xsd, xml);
    }, [entity, onEdit]);

    const handleDelete = React.useCallback((model: string) => {
        onDelete(entity || null, model);
    }, [entity, onDelete]);

    const handleDownload = React.useCallback(() => {
        onDownload(entity || null);
    }, [entity, onDownload]);

    return (
        <TableRow>
            <TableCell key={0} align="left">
                <Tooltip placement="top" title="Download configuration files of this entity">
                    <IconButton size="small" onClick={handleDownload}>
                        <DownloadIcon />
                    </IconButton>
                </Tooltip>
            </TableCell>
            {headers.map((id: string, i: number) => {
                const param = parameters.find((p: Parameter) => p.id === id);
                if (param == null) {
                    return <TableCell key={i+1} align="center" />;
                }
                return (
                    <TableCell key={i+1} align="center">
                        <Parameters
                            parameter={param}
                            templates={templates}
                            entityType={entityType}
                            onEdit={handleEdit}
                            onDelete={handleDelete}
                            onSelect={param.id === "infrastructure" ? handleSelect : undefined}
                            enumeration={enumerations.find((e: Enum) => e.id === param.type)}
                        />
                    </TableCell>
                );
            })}
            <TableCell key={headers.length + 1} align="right">
                <Tooltip placement="top" title="Remove this entity">
                    <IconButton size="small" onClick={onRemove}>
                        <DeleteIcon />
                    </IconButton>
                </Tooltip>
            </TableCell>
        </TableRow>
    );
};


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
                    {list.elements.map((c: Component, i: number) => (
                        <ProjectListEntityItem
                            key={i}
                            entity={c.parameters.find((p: Parameter) => p.id === "name")?.value}
                            onEdit={props.onEdit}
                            onDelete={props.onDelete}
                            onDownload={props.onDownload}
                            onSelect={props.onSelect}
                            onRemove={() => removeListItem(i)}
                            headers={headers}
                            parameters={c.parameters}
                            templates={templates}
                            enumerations={enums}
                        />
                    ))}
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
