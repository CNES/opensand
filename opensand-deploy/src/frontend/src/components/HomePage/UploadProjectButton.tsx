import React from 'react';

import Button from '@mui/material/Button';
import Dialog from '@mui/material/Dialog';
import DialogActions from '@mui/material/DialogActions';
import DialogContent from '@mui/material/DialogContent';
import DialogContentText from '@mui/material/DialogContentText';
import DialogTitle from '@mui/material/DialogTitle';
import FormControl from '@mui/material/FormControl';
import FormHelperText from '@mui/material/FormHelperText';
import TextField from '@mui/material/TextField';

import {uploadProject} from '../../api';
import {useSelector, useDispatch} from '../../redux';
import {useProject} from '../../utils/hooks';


const fileInputId = "project-upload-file-input-id";


const UploadProjectButton = () => {
    const status = useSelector((state) => state.project.status);
    const dispatch = useDispatch();
    const goToProject = useProject(dispatch);

    const [open, setOpen] = React.useState<boolean>(false);
    const [name, setName] = React.useState<string>("");
    const [nameError, setNameError] = React.useState<boolean>(false);
    const [file, setFile] = React.useState<File | undefined>(undefined);
    const [fileError, setFileError] = React.useState<boolean>(false);

    const handleOpen = React.useCallback(() => {
        setName("");
        setOpen(true);
    }, []);

    const handleClose = React.useCallback(() => {
        setNameError(false);
        setFile(undefined);
        setFileError(false);
        setOpen(false);
    }, []);

    const changeName = React.useCallback((event: React.ChangeEvent<HTMLInputElement>) => {
        setName(event.target.value);
    }, []);

    const changeFile = React.useCallback((event: React.ChangeEvent<HTMLInputElement>) => {
        const selected = event.target.files ? event.target.files[0] : null;
        if (selected == null) {
            setFile(undefined);
        } else {
            setFile(selected);
        }
    }, []);

    const handleValidate = React.useCallback((event: React.FormEvent<HTMLFormElement>) => {
        event.preventDefault();
        if (name !== "" && file != null) {
            dispatch(uploadProject({project: name, archive: file}));
            handleClose();
        } else {
            setNameError(name === "");
            setFileError(file == null);
        }
    }, [name, file, handleClose, dispatch]);

    React.useEffect(() => {
        if (name && status === "created") {
            setName("");
            goToProject(name);
        }
    }, [status, name, goToProject]);

    return (
        <React.Fragment>
            <Button color="primary" onClick={handleOpen}>Upload</Button>
            <Dialog open={open} onClose={handleClose}>
                <form onSubmit={handleValidate}>
                    <DialogTitle>Upload Project</DialogTitle>
                    <DialogContent>
                        <DialogContentText>Please provide a name and a file for the new project</DialogContentText>
                        <FormControl error={nameError || fileError} fullWidth>
                            <TextField
                                autoFocus
                                margin="dense"
                                label="Project Name"
                                value={name}
                                onChange={changeName}
                                fullWidth
                            />
                            {nameError && <FormHelperText>Mandatory field</FormHelperText>}
                            <label htmlFor={fileInputId}>
                                <Button variant="outlined" component="span" color={file ? "primary" : fileError ? "secondary" : "inherit"}>
                                    {file ? file.name : "Select a file to upload"}
                                </Button>
                            </label>
                            <input
                                id={fileInputId}
                                type="file"
                                accept=".tar.gz"
                                onChange={changeFile}
                                hidden
                            />
                            {fileError && <FormHelperText>Mandatory field</FormHelperText>}
                        </FormControl>
                    </DialogContent>
                    <DialogActions>
                        <Button onClick={handleClose} color="primary">Cancel</Button>
                        <Button type="submit" color="primary">Upload</Button>
                    </DialogActions>
                </form>
            </Dialog>
        </React.Fragment>
    );
};


export default UploadProjectButton;
