import React from 'react';

import Button from '@mui/material/Button';
import Dialog from '@mui/material/Dialog';
import DialogActions from '@mui/material/DialogActions';
import DialogContent from '@mui/material/DialogContent';
import DialogContentText from '@mui/material/DialogContentText';
import DialogTitle from '@mui/material/DialogTitle';
import Grid from '@mui/material/Grid';

import CardAdd from '../common/CardAdd';
import CreateProjectButton from './CreateProjectButton';
import UploadProjectButton from './UploadProjectButton';
import ProjectCard from './ProjectCard';

import {deleteProject, listProjects} from '../../api';
import {useSelector, useDispatch} from '../../redux';


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

    return (
        <Grid container spacing={1} sx={{p: 2}}>
            <Grid item xs={12} sm={6} lg={4} xl={3}>
                <CardAdd title="project" subtitle="Manage a new OpenSAND platform">
                    <CreateProjectButton />
                    <UploadProjectButton />
                </CardAdd>
            </Grid>
            {projects.map((p: string, i: number) => (
				<Grid item xs={12} sm={6} lg={4} xl={3} key={p}>
					<ProjectCard
						project={p}
						onDelete={setProjectToDelete}
					/>
				</Grid>
			))}
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
        </Grid>
    );
};


interface Props {
}


export default Projects;
