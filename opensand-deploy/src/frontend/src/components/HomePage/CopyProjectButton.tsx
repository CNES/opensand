import React from 'react';

import Button from '@mui/material/Button';

import SingleFieldDialog from '../common/SingleFieldDialog';

import {copyProject} from '../../api';
import {useSelector, useDispatch} from '../../redux';
import {useProject} from '../../utils/hooks';


const CopyProjectButton: React.FC<Props> = (props) => {
    const {project, onClose, onClick} = props;
    const status = useSelector((state) => state.project.status);
    const dispatch = useDispatch();
    const goToProject = useProject(dispatch);

    const [createdProject, setCreatedProjectName] = React.useState<string>("");

    const doCreateProject = React.useCallback((projectName: string) => {
        if (project && projectName) {
            setCreatedProjectName(projectName);
            dispatch(copyProject({project, name: projectName}));
            onClose();
        }
    }, [dispatch, project, onClose]);

    React.useEffect(() => {
        if (createdProject && status === "created") {
            setCreatedProjectName("");
            goToProject(createdProject);
        }
    }, [status, createdProject, goToProject]);

    return (
        <React.Fragment>
            <Button color="secondary" variant="contained" onClick={onClick}>
                Copy
            </Button>
            <SingleFieldDialog
                open={Boolean(project)}
                title="New Project"
                description="Please enter the name of your new project."
                fieldLabel="Project Name"
                onValidate={doCreateProject}
                onClose={onClose}
            />
        </React.Fragment>
    );
};


interface Props {
    project: string | false;
    onClick: () => void;
    onClose: () => void;
}


export default CopyProjectButton;
