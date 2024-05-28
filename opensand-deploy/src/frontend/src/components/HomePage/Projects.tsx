import React from 'react';

import Grid from '@mui/material/Grid';
import IconButton from '@mui/material/IconButton';
import Snackbar from '@mui/material/Snackbar';
import Stack from '@mui/material/Stack';

import CloseIcon from '@mui/icons-material/Close';

import CopyProjectButton from './CopyProjectButton';
import CreateProjectButton from './CreateProjectButton';
import DeleteProjectButton from './DeleteProjectButton';
import DownloadProjectButton from './DownloadProjectButton';
import UploadProjectButton from './UploadProjectButton';
import ProjectCard from './ProjectCard';

import {listProjects} from '../../api';
import {useSelector, useDispatch} from '../../redux';


type ProjectAction = (project: string) => void;


const Projects: React.FC<Props> = (props) => {
    const projects = useSelector((state) => state.project.projects);
    const loading = useSelector((state) => state.project.status);
    const dispatch = useDispatch();

    const [selectProject, setAction] = React.useState<ProjectAction | null>(null);
    const [selectedProject, setSelectedProject] = React.useState<string | null>(null);
    const [actionName, setActionName] = React.useState<"copy" | "delete" | "download" | "open">("open");

    const clearAction = React.useCallback((event?: React.SyntheticEvent | Event, reason?: string) => {
        if (reason !== "clickaway") {
            setAction(null);
            setActionName("open");
            setSelectedProject(null);
        }
    }, []);

    const setActionToDownload = React.useCallback(() => {
        setAction(() => setSelectedProject);
        setActionName("download");
    }, []);

    const setActionToCopy = React.useCallback(() => {
        setAction(() => setSelectedProject);
        setActionName("copy");
    }, []);

    const setActionToDelete = React.useCallback(() => {
        setAction(() => setSelectedProject);
        setActionName("delete");
    }, []);

    React.useEffect(() => {
        if (loading === "idle") {
            dispatch(listProjects());
        }
    }, [loading, dispatch]);

    return (
        <React.Fragment>
            <Grid container spacing={1} sx={{p: 2}}>
                {projects.map((p: string, i: number) => (
                    <Grid item xs={12} sm={6} lg={4} xl={3} key={p}>
                        <ProjectCard
                            project={p}
                            action={selectProject}
                            actionName={actionName}
                        />
                    </Grid>
                ))}
            </Grid>
            <Stack direction="row" justifyContent="center" alignItems="center" spacing={1} mt={1} mb={3}>
                <CreateProjectButton />
                <UploadProjectButton />
                <DownloadProjectButton
                    project={selectedProject != null && actionName === "download" && selectedProject}
                    onClose={clearAction}
                    onClick={setActionToDownload}
                />
                <CopyProjectButton
                    project={selectedProject != null && actionName === "copy" && selectedProject}
                    onClose={clearAction}
                    onClick={setActionToCopy}
                />
                <DeleteProjectButton
                    project={selectedProject != null && actionName === "delete" && selectedProject}
                    onClose={clearAction}
                    onClick={setActionToDelete}
                />
            </Stack>
            <Snackbar
                open={Boolean(selectProject)}
                onClose={clearAction}
                message={`Please select a project to ${actionName}`}
                action={(
                    <IconButton size="small" color="inherit" onClick={clearAction}>
                        <CloseIcon fontSize="small" />
                    </IconButton>
                )}
            />
        </React.Fragment>
    );
};


interface Props {
}


export default Projects;
