import React from 'react';
import {useHistory} from 'react-router-dom';

import Accordion from "@material-ui/core/Accordion";
import AccordionSummary from "@material-ui/core/AccordionSummary";
import AccordionDetails from "@material-ui/core/AccordionDetails";
import Box from "@material-ui/core/Box";
import IconButton from '@material-ui/core/IconButton';
import Paper from '@material-ui/core/Paper';
import Table from '@material-ui/core/Table';
import TableBody from '@material-ui/core/TableBody';
import TableCell from '@material-ui/core/TableCell';
import TableContainer from '@material-ui/core/TableContainer';
import TableHead from '@material-ui/core/TableHead';
import TableRow from '@material-ui/core/TableRow';
import Typography from "@material-ui/core/Typography";

import AddIcon from '@material-ui/icons/AddCircleOutline';
import DeleteIcon from '@material-ui/icons/HighlightOff';
import EditIcon from '@material-ui/icons/Edit';
import ExpandMoreIcon from "@material-ui/icons/ExpandMore";

import {listProjectTemplates, deleteProjectTemplate, ITemplatesContent} from '../../api';
import {componentStyles} from '../../utils/theme';
import {Model, Component, Parameter, Enum} from '../../xsd/model';

import SingleFieldDialog from '../common/SingleFieldDialog';


interface Props {
    project: Model;
}


interface TableProps {
    template: string;
    templates?: string[];
    onCreate: (templateName: string) => void;
    onEdit: (templateName: string) => void;
    onRemove: (templateName: string) => void;
}

interface RowProps {
    templates?: string[];
    onEdit: (templateName: string) => void;
    onRemove: (templateName: string) => void;
}


const TemplatesTableBody = (props: RowProps) => {
    const {onEdit, onRemove} = props;
    const templates = props.templates || [];

    return (
        <TableBody>
            {templates.map((templateFile: string, i: number) => (
                <TableRow key={i}>
                    <TableCell align="left">
                        <IconButton size="small" onClick={onEdit.bind(this, templateFile)}>
                            <EditIcon />
                        </IconButton>
                    </TableCell>
                    <TableCell align="center">
                        {templateFile}
                    </TableCell>
                    <TableCell align="right">
                        <IconButton size="small" onClick={onRemove.bind(this, templateFile)}>
                            <DeleteIcon />
                        </IconButton>
                    </TableCell>
                </TableRow>
            ))}
        </TableBody>
    );
};


const TemplatesTable = (props: TableProps) => {
    const {template, templates, onCreate, onEdit, onRemove} = props;

    const [open, setOpen] = React.useState<boolean>(false);
    const classes = componentStyles();

    const handleOpen = React.useCallback(() => {
        setOpen(true);
    }, [setOpen]);

    const handleClose = React.useCallback(() => {
        setOpen(false);
    }, [setOpen]);

/* FIXME: see addTemplate below
    const handleCreate = React.useCallback((name: string) => {
        setOpen(false);
        onCreate(name.endsWith(".xml") ? name : name + ".xml");
    }, [setOpen, onCreate]);
*/
    const handleCreate = React.useCallback((name: string) => {
        setOpen(false);
        onEdit(name.endsWith(".xml") ? name : name + ".xml");
    }, [setOpen, onEdit]);

    return (
        <Accordion defaultExpanded={false}>
            <AccordionSummary expandIcon={<ExpandMoreIcon />}>
                <Typography className={classes.heading}>{template}</Typography>
            </AccordionSummary>
            <AccordionDetails>
                <TableContainer>
                    <Table>
                        <TableHead>
                            <TableRow>
                                <TableCell align="left" />
                                <TableCell align="center">
                                    Template Name
                                </TableCell>
                                <TableCell align="right">
                                    <IconButton size="small" onClick={handleOpen}>
                                        <AddIcon />
                                    </IconButton>
                                </TableCell>
                            </TableRow>
                        </TableHead>
                        <TemplatesTableBody templates={templates} onEdit={onEdit} onRemove={onRemove}/>
                    </Table>
                </TableContainer>
                <SingleFieldDialog
                    open={open}
                    title={`New ${template} Template`}
                    description="Please enter the name of your new template."
                    fieldLabel="Template Name"
                    onValidate={handleCreate}
                    onClose={handleClose}
                />
            </AccordionDetails>
        </Accordion>
    );
};


const Templates = (props: Props) => {
    const projectComponent = props.project.root.children.find((c: Component) => c.id === "project");
    const projectName = projectComponent?.parameters.find((p: Parameter) => p.id === "name")?.value;

    const templateTypes = props.project.environment.enums;
    const [templates, setTemplates] = React.useState<ITemplatesContent>({});
    const classes = componentStyles();
    const history = useHistory();

    /* FIXME: should we keep this or only use `editTemplate` */
    const addTemplate = React.useCallback((templateFile: string, templateName: string) => {
        const templateNames = templates.hasOwnProperty(templateFile)
            ? [...templates[templateFile], templateName]
            : [templateName];
        setTemplates({...templates, [templateFile]: templateNames});
    }, [setTemplates, templates]);

    const removeTemplate = React.useCallback((templateFile: string, templateName: string) => {
        if (templates.hasOwnProperty(templateFile)) {
            const templateNames = templates[templateFile].filter((name: string) => name !== templateName);
            setTemplates({...templates, [templateFile]: templateNames});
        }
        if (projectName != null) {
            deleteProjectTemplate(console.log, console.log, projectName, templateFile, templateName);
        }
    }, [setTemplates, templates, projectName]);

    const editTemplate = React.useCallback((templateFile: string, templateName: string) => {
        if (projectName != null) {
            history.push({
                pathname: "/edit/" + projectName,
                search: "?url=template/" + templateFile + "/" + templateName + "&xsd=" + templateFile,
            });
        }
    }, [history, projectName]);

    React.useEffect(() => {
        if (projectName != null) {
            listProjectTemplates(setTemplates, console.log, projectName);
        }
        return () => {setTemplates({});};
    }, [setTemplates, projectName]);

    return (
        <Paper elevation={0} className={classes.root}>
            {templateTypes.map((e: Enum, i: number) => (
                <Accordion key={i} defaultExpanded={true}>
                    <AccordionSummary expandIcon={<ExpandMoreIcon />}>
                        <Typography className={classes.heading}>{e.name}</Typography>
                        <Typography className={classes.secondaryHeading}>{e.description}</Typography>
                    </AccordionSummary>
                    <AccordionDetails>
                        <Box width="100%">
                            {e.values.filter(v => v !== "").map((templateName: string, i: number) => (
                                <TemplatesTable
                                    key={i}
                                    template={templateName}
                                    templates={templates[templateName]}
                                    onCreate={addTemplate.bind(this, templateName)}
                                    onEdit={editTemplate.bind(this, templateName)}
                                    onRemove={removeTemplate.bind(this, templateName)}
                                />
                            ))}
                        </Box>
                    </AccordionDetails>
                </Accordion>
            ))}
        </Paper>
    );
};


export default Templates;
