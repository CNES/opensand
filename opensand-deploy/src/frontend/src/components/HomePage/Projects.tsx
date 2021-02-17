import React from 'react';

import Card from '@material-ui/core/Card';
import CardActions from '@material-ui/core/CardActions';
import CardContent from '@material-ui/core/CardContent';
import Typography from '@material-ui/core/Typography';

import {makeStyles, Theme} from '@material-ui/core/styles';

import {listProjects} from '../../api';
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
    const [projects, setProjects] = React.useState<string[]>([]);
    const classes = useStyles();

    const forceRedraw = React.useCallback(() => {
        listProjects(setProjects, sendError);
    }, [setProjects]);

    React.useEffect(() => {
        listProjects(setProjects, sendError);
        return () => {setProjects([]);}
    }, [setProjects]);

    const projectsCards = projects.map((p: string, i: number) => (
        <ProjectCard
            key={i+1}
            className={classes.card}
            project={p}
            onReload={forceRedraw}
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
        </div>
    );
};


export default Projects;
