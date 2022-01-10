import React from 'react';
import {useHistory} from 'react-router-dom';

import Box from '@material-ui/core/Box';
import Card from '@material-ui/core/Card';
import CardActions from '@material-ui/core/CardActions';
import CardContent from '@material-ui/core/CardContent';
import Typography from '@material-ui/core/Typography';

import {copyProject, IApiSuccess} from '../../api';
import {sendError} from '../../utils/dispatcher';

import CardButton from './CardButton';
import SingleFieldDialog from '../common/SingleFieldDialog';


interface Props {
    project: string;
    className: string;
    onDelete: (name: string) => void;
}


const ProjectCard = (props: Props) => {
    const {project, className, onDelete} = props;
    const history = useHistory();

    const [open, setOpen] = React.useState<boolean>(false);

    const handleOpen = React.useCallback(() => {
        setOpen(true);
    }, [setOpen]);

    const handleClose = React.useCallback(() => {
        setOpen(false);
    }, [setOpen]);


    const openProject = React.useCallback(() => {
        history.push("/project/" + project);
    }, [history, project]);

    const removeProject = React.useCallback(() => {
        onDelete(project);
    }, [onDelete, project]);

    const downloadProject = React.useCallback(() => {
        const form = document.createElement("form") as HTMLFormElement;
        form.method = "post";
        form.action = "/api/project/" + project;
        document.body.appendChild(form);
        form.submit();
        document.body.removeChild(form);
    }, [project]);

    const handleCreatedProject = React.useCallback((success: IApiSuccess) => {
        setOpen(false);
        history.push("/project/" + success.status);
    }, [setOpen, history]);

    const doCreateProject = React.useCallback((projectName: string) => {
        if (projectName !== "") {
            copyProject(handleCreatedProject, sendError, project, projectName);
        }
    }, [project, handleCreatedProject]);

    return (
        <React.Fragment>
            <Card className={className} onClick={openProject}>
                <CardContent>
                    <Typography>
                        {project}
                    </Typography>
                </CardContent>
                <CardActions>
                    <Box flexGrow="1" />
                    <CardButton title="Open" onClick={openProject} />
                    <CardButton title="Download" onClick={downloadProject} />
                    <CardButton title="Copy" onClick={handleOpen} />
                    <CardButton title="Delete" onClick={removeProject} />
                </CardActions>
            </Card>
            <SingleFieldDialog
                open={open}
                title="New Project"
                description="Please enter the name of your new project."
                fieldLabel="Project Name"
                onValidate={doCreateProject}
                onClose={handleClose}
            />
        </React.Fragment>
    );
};


export default ProjectCard;
