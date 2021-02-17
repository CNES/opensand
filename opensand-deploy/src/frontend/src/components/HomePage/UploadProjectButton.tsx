import React from 'react';
import {useHistory} from 'react-router-dom';

import Button from '@material-ui/core/Button';
import Dialog from '@material-ui/core/Dialog';
import DialogActions from '@material-ui/core/DialogActions';
import DialogContent from '@material-ui/core/DialogContent';
import DialogContentText from '@material-ui/core/DialogContentText';
import DialogTitle from '@material-ui/core/DialogTitle';
import FormControl from '@material-ui/core/FormControl';
import FormHelperText from '@material-ui/core/FormHelperText';
import TextField from '@material-ui/core/TextField';

import {uploadProject, IApiSuccess} from '../../api';
import {sendError} from '../../utils/dispatcher';


const fileInputId = "project-upload-file-input-id";


const UploadProjectButton = () => {
    const [open, setOpen] = React.useState<boolean>(false);
    const [name, setName] = React.useState<string>("");
    const [nameError, setNameError] = React.useState<boolean>(false);
    const [file, setFile] = React.useState<File | undefined>(undefined);
    const [fileError, setFileError] = React.useState<boolean>(false);
    const history = useHistory();

    const handleOpen = React.useCallback(() => {
        setOpen(true);
    }, [setOpen]);

    const handleClose = React.useCallback(() => {
        setName("");
        setNameError(false);
        setFile(undefined);
        setFileError(false);
        setOpen(false);
    }, [setOpen, setName, setNameError, setFile, setFileError]);

    const changeName = React.useCallback((event: React.ChangeEvent<HTMLInputElement>) => {
        setName(event.target.value);
    }, [setName]);

    const changeFile = React.useCallback((event: React.ChangeEvent<HTMLInputElement>) => {
        const selected = event.target.files ? event.target.files[0] : null;
        if (selected == null) {
            setFile(undefined);
        } else {
            setFile(selected);
        }
    }, [setFile]);

    const handleCreatedProject = React.useCallback((project: string, success: IApiSuccess) => {
        if (success.status === "OK") {
            history.push("/project/" + project);
        }
    }, [history]);

    const handleValidate = React.useCallback((event: React.FormEvent<HTMLFormElement>) => {
        event.preventDefault();
        if (name !== "" && file != null) {
            uploadProject(handleCreatedProject.bind(this, name), sendError, name, file);
            handleClose();
        } else {
            setNameError(name === "");
            setFileError(file == null);
        }
    }, [name, file, handleClose, setNameError, setFileError, handleCreatedProject]);

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
                                <Button variant="outlined" component="span" color={file ? "primary" : fileError ? "secondary" : "default"}>
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
