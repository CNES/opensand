import React from 'react';

import Avatar from '@mui/material/Avatar';
import Box from '@mui/material/Box';
import Button from '@mui/material/Button';
import Card from '@mui/material/Card';
import CardActionArea from '@mui/material/CardActionArea';
import CardActions from '@mui/material/CardActions';
import CardHeader from '@mui/material/CardHeader';

import OpenIcon from '@mui/icons-material/Lan';

import {useDispatch} from '../../redux';
import {useProject} from '../../utils/hooks';


const ProjectCard = (props: Props) => {
    const {project, action, actionName} = props;
    const dispatch = useDispatch();
    const goToProject = useProject(dispatch);

    const selectProject = React.useCallback(() => {
        if (action) {
            action(project);
        } else {
            goToProject(project);
        }
    }, [action, project, goToProject]);

    return (
        <Card variant="outlined">
            <CardActionArea onClick={selectProject}>
                <CardHeader
                    avatar={<Avatar><OpenIcon /></Avatar>}
                    title={project}
                />
                <CardActions>
                    <Box flexGrow={1} />
                    <Button color="primary" variant="outlined" onClick={selectProject}>{actionName}</Button>
                </CardActions>
            </CardActionArea>
        </Card>
    );
};


interface Props {
    project: string;
    action: ((project: string) => void) | null;
    actionName: string;
}


export default ProjectCard;
