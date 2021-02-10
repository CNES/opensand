import React from 'react';
import {useHistory} from 'react-router-dom';

import Box from '@material-ui/core/Box';
import Card from '@material-ui/core/Card';
import CardActions from '@material-ui/core/CardActions';
import CardContent from '@material-ui/core/CardContent';
import Button from '@material-ui/core/Button';
import Typography from '@material-ui/core/Typography';

import {deleteProject, IApiSuccess} from '../../api';
import {sendError} from '../../utils/dispatcher';


interface Props {
    project: string;
    className: string;
    onReload: () => void;
}


const ProjectCard = (props: Props) => {
    const {project, className, onReload} = props;
    const history = useHistory();

    const openProject = React.useCallback(() => {
        history.push("/project/" + project);
    }, [history, project]);

    const onSuccess = React.useCallback((status: IApiSuccess) => {
        onReload();
    }, [onReload]);

    const removeProject = React.useCallback(() => {
        deleteProject(onSuccess, sendError, project);
    }, [onSuccess, project]);

    return (
        <Card className={className} onClick={openProject}>
            <CardContent>
                <Typography>
                    {project}
                </Typography>
            </CardContent>
            <CardActions>
                <Box flexGrow="1" />
                <Button color="primary" onClick={openProject}>Open</Button>
                <Button color="primary" disabled>Download</Button>
                <Button color="primary" onClick={removeProject}>Delete</Button>
            </CardActions>
        </Card>
    );
};


export default ProjectCard;
