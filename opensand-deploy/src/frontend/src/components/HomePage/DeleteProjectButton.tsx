import React from 'react';

import Button from '@mui/material/Button';
import Dialog from '@mui/material/Dialog';
import DialogActions from '@mui/material/DialogActions';
import DialogContent from '@mui/material/DialogContent';
import DialogContentText from '@mui/material/DialogContentText';
import DialogTitle from '@mui/material/DialogTitle';

import {deleteProject} from '../../api';
import {useDispatch} from '../../redux';



const DeleteProjectButton: React.FC<Props> = (props) => {
    const {project, onClose, onClick} = props;
    const dispatch = useDispatch();

    const doRemoveProject = React.useCallback(() => {
        if (project) {
            dispatch(deleteProject({project}));
        }
        onClose();
    }, [dispatch, project, onClose]);

    return (
        <React.Fragment>
            <Button color="error" variant="contained" onClick={onClick}>
                Delete
            </Button>
            <Dialog open={Boolean(project)} onClose={onClose}>
                <DialogTitle>Delete a project</DialogTitle>
                <DialogContent>
                    <DialogContentText>You're about to delete project {project}!</DialogContentText>
                    <DialogContentText>This action can't be reverted, are you sure?</DialogContentText>
                </DialogContent>
                <DialogActions>
                    <Button onClick={onClose} color="primary">No, Keep it</Button>
                    <Button onClick={doRemoveProject} color="primary">Yes, Delete {project}</Button>
                </DialogActions>
            </Dialog>
        </React.Fragment>
    );
};


interface Props {
    project: string | false;
    onClick: () => void;
    onClose: () => void;
}


export default DeleteProjectButton;
