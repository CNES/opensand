import React from 'react';

import Button from '@material-ui/core/Button';
import Card from '@material-ui/core/Card';
import CardActions from '@material-ui/core/CardActions';
import CardContent from '@material-ui/core/CardContent';
import Dialog from '@material-ui/core/Dialog';
import DialogActions from '@material-ui/core/DialogActions';
import DialogContent from '@material-ui/core/DialogContent';
import DialogContentText from '@material-ui/core/DialogContentText';
import DialogTitle from '@material-ui/core/DialogTitle';
import Typography from '@material-ui/core/Typography';

import {makeStyles, Theme} from '@material-ui/core/styles';

import {deleteProject, listProjects, IApiSuccess, IProjectsContent} from '../../api';
import {sendError} from '../../utils/dispatcher';

import CreateProjectButton from './CreateProjectButton';
import UploadProjectButton from './UploadProjectButton';
import ProjectCard from './ProjectCard';


const useStyles = makeStyles((theme: Theme) => ({
    root: {
        width: "96%",
        marginLeft: "2%",
        marginRight: "2%",
    },
    card: {
        width: "100%",
        marginTop: "2%",
        marginBottom: "2%",
    },
}));


const Projects = () => {
    const classes = useStyles();

    const [projects, setProjects] = React.useState<string[]>([]);
    const [deleteThis, setProjectToDelete] = React.useState<string | null>(null);

    const storeProjects = React.useCallback((response: IProjectsContent) => {
        setProjects(response.projects);
    }, [setProjects]);

    const forceRedraw = React.useCallback((status: IApiSuccess) => {
        listProjects(storeProjects, sendError);
    }, [storeProjects]);

    const clearDeleteProject = React.useCallback(() => {
        setProjectToDelete(null);
    }, []);

    const handleDeleteProject = React.useCallback(() => {
        if (deleteThis != null) {
            deleteProject(forceRedraw, sendError, deleteThis);
        }
        clearDeleteProject();
    }, [forceRedraw, deleteThis, clearDeleteProject]);

    React.useEffect(() => {
        listProjects(storeProjects, sendError);
        return () => {setProjects([]);}
    }, [storeProjects]);

    const projectsCards = projects.map((p: string, i: number) => (
        <ProjectCard
            key={i+1}
            className={classes.card}
            project={p}
            onDelete={setProjectToDelete}
        />
    ));

    return (
        <div className={classes.root}>
            <Card key={0} className={classes.card}>
                <CardContent>
                    <Typography>
                        New Project
                    </Typography>
                </CardContent>
                <CardActions>
                    <CreateProjectButton />
                    <UploadProjectButton />
                </CardActions>
            </Card>
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
        </div>
    );
};


export default Projects;
