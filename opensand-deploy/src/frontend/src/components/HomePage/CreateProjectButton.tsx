import React from 'react';

import Button from '@mui/material/Button';

import {createProject} from '../../api';
import {useSelector, useDispatch} from '../../redux';
import {useOpen, useProject} from '../../utils/hooks';

import SingleFieldDialog from '../common/SingleFieldDialog';


const CreateProjectButton: React.FC<Props> = (props) => {
    const status = useSelector((state) => state.project.status);
    const dispatch = useDispatch();
    const goToProject = useProject(dispatch);

    const [projectName, setProjectName] = React.useState<string>("");
    const [open, handleOpen, handleClose] = useOpen();

    const doCreateProject = React.useCallback((project: string) => {
        if (project) {
            setProjectName(project);
            dispatch(createProject({project}));
        }
        handleClose();
    }, [dispatch, handleClose]);

    React.useEffect(() => {
        if (projectName && status === "created") {
            setProjectName("");
            goToProject(projectName);
        }
    }, [status, projectName, goToProject]);

    return (
        <React.Fragment>
            <Button color="success" variant="contained" onClick={handleOpen}>
                Create
            </Button>
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
}


export default CreateProjectButton;
