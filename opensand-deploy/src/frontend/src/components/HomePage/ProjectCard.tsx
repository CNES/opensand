import React from 'react';

import Avatar from '@mui/material/Avatar';
import Box from '@mui/material/Box';
import Card from '@mui/material/Card';
import CardActionArea from '@mui/material/CardActionArea';
import CardActions from '@mui/material/CardActions';
import CardContent from '@mui/material/CardContent';
import CardHeader from '@mui/material/CardHeader';
import Typography from '@mui/material/Typography';

import OpenIcon from '@mui/icons-material/Lan';

import CardButton from './CardButton';
import SingleFieldDialog from '../common/SingleFieldDialog';

import {copyProject} from '../../api';
import {useSelector, useDispatch} from '../../redux';
import {useOpen, useProject} from '../../utils/hooks';


const ProjectCard = (props: Props) => {
    const {project, onDelete} = props;

    const status = useSelector((state) => state.project.status);
    const dispatch = useDispatch();
    const goToProject = useProject(dispatch);

    const [createdProject, setCreatedProjectName] = React.useState<string>("");
    const [open, handleOpen, handleClose] = useOpen();

    const openProject = React.useCallback(() => {
        goToProject(project);
    }, [goToProject, project]);

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

    const doCreateProject = React.useCallback((projectName: string) => {
        if (projectName) {
            setCreatedProjectName(projectName);
            dispatch(copyProject({project, name: projectName}));
            handleClose();
        }
    }, [dispatch, project, handleClose]);

    React.useEffect(() => {
        if (createdProject && status === "created") {
            setCreatedProjectName("");
            goToProject(createdProject);
        }
    }, [status, createdProject, goToProject]);

    return (
        <React.Fragment>
            <Card variant="outlined">
                <CardActionArea onClick={openProject}>
                    <CardHeader
                        avatar={<Avatar><OpenIcon /></Avatar>}
                        title="Project"
                    />
					<CardContent>
						<Box textAlign="center">
							<Typography variant="button">
								{project}
							</Typography>
						</Box>
					</CardContent>
                </CardActionArea>
                <CardActions sx={{flexDirection: "column"}}>
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


interface Props {
    project: string;
    onDelete: (name: string) => void;
}


export default ProjectCard;
