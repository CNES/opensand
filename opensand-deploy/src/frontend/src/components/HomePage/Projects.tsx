import React from 'react';

import Button from '@mui/material/Button';
import CardActions from '@mui/material/CardActions';
import CardContent from '@mui/material/CardContent';
import Dialog from '@mui/material/Dialog';
import DialogActions from '@mui/material/DialogActions';
import DialogContent from '@mui/material/DialogContent';
import DialogContentText from '@mui/material/DialogContentText';
import DialogTitle from '@mui/material/DialogTitle';
import Typography from '@mui/material/Typography';

import {styled} from '@mui/material/styles';

import CreateProjectButton from './CreateProjectButton';
import UploadProjectButton from './UploadProjectButton';
import ProjectCard, {LargeCard} from './ProjectCard';

import {deleteProject, listProjects} from '../../api';
import {useSelector, useDispatch} from '../../redux';


const Root = styled('div')({
    width: "96%",
    marginLeft: "2%",
    marginRight: "2%",
});


const Projects: React.FC<Props> = (props) => {
    const projects = useSelector((state) => state.project.projects);
    const loading = useSelector((state) => state.project.status);
    const dispatch = useDispatch();

    const [deleteThis, setProjectToDelete] = React.useState<string | null>(null);

    const clearDeleteProject = React.useCallback(() => {
        setProjectToDelete(null);
    }, []);

    const handleDeleteProject = React.useCallback(() => {
        setProjectToDelete((removable: string | null) => {
            if (removable != null) {
                dispatch(deleteProject({project: removable}));
            }
            return null;
        });
    }, [dispatch]);

    React.useEffect(() => {
        if (loading === "idle") {
            dispatch(listProjects());
        }
    }, [loading, dispatch]);

    const projectsCards = projects.map((p: string, i: number) => (
        <ProjectCard
            key={p}
            project={p}
            onDelete={setProjectToDelete}
        />
    ));

    return (
        <Root>
            <LargeCard key="app/root">
                <CardContent>
                    <Typography>
                        New Project
                    </Typography>
                </CardContent>
                <CardActions>
                    <CreateProjectButton />
                    <UploadProjectButton />
                </CardActions>
            </LargeCard>
            {projectsCards}
            <Dialog open={deleteThis != null} onClose={clearDeleteProject}>
                <DialogTitle>Delete a project</DialogTitle>
                <DialogContent>
                    <DialogContentText>You're about to delete project {deleteThis}!</DialogContentText>
                    <DialogContentText>This action can't be reverted, are you sure?</DialogContentText>
                </DialogContent>
                <DialogActions>
                    <Button onClick={clearDeleteProject} color="primary">No, Keep it</Button>
                    <Button onClick={handleDeleteProject} color="primary">Yes, Delete {deleteThis}</Button>
                </DialogActions>
            </Dialog>
        </Root>
    );
};


interface Props {
}


export default Projects;
