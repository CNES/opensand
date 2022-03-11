import React from 'react';

import Button from '@mui/material/Button';
import Checkbox from '@mui/material/Checkbox';
import Dialog from '@mui/material/Dialog';
import DialogActions from '@mui/material/DialogActions';
import DialogContent from '@mui/material/DialogContent';
import DialogContentText from '@mui/material/DialogContentText';
import DialogTitle from '@mui/material/DialogTitle';
import FormControlLabel from '@mui/material/FormControlLabel';
import TextField from '@mui/material/TextField';


const DeployEntityDialog = (props: Props) => {
    const {open, onValidate, onClose} = props;

    const [password, setPassword] = React.useState<string>("");
    const [passphrase, setPassphrase] = React.useState<boolean>(false);

    const changePassword = React.useCallback((event: React.ChangeEvent<HTMLInputElement>) => {
        setPassword(event.target.value);
    }, []);

    const changePassphrase = React.useCallback((event: React.ChangeEvent<HTMLInputElement>) => {
        setPassphrase(event.target.checked);
    }, []);

    const reset = React.useCallback(() => {
        setPassword("");
        setPassphrase(false);
    }, []);

    const handleClose = React.useCallback(() => {
        onClose();
        reset();
    }, [reset, onClose]);

    const handleValidate = React.useCallback((event: React.FormEvent<HTMLFormElement>) => {
        event.preventDefault();
        onValidate && onValidate(password, passphrase);
        handleClose();
    }, [onValidate, handleClose, password, passphrase]);

    return (
        <Dialog open={open} onClose={handleClose}>
            <form onSubmit={handleValidate}>
                <DialogTitle>Deploy an Entity</DialogTitle>
                <DialogContent>
                    <DialogContentText>Please specify the SSH password to upload files and/or connect to the entity</DialogContentText>
                    <FormControlLabel
                        control={<Checkbox checked={passphrase} onChange={changePassphrase} name="passphrase" />}
                        label="Passphrase for an SSH private key"
                    />
                    <TextField
                        autoFocus
                        margin="dense"
                        label={passphrase ? "Passphrase" : "Password"}
                        type="password"
                        value={password}
                        onChange={changePassword}
                        fullWidth
                    />
                </DialogContent>
                <DialogActions>
                    <Button onClick={handleClose} color="primary">Cancel</Button>
                    <Button type="submit" color="primary">Deploy</Button>
                </DialogActions>
            </form>
        </Dialog>
    );
};


interface Props {
    open: boolean;
    onValidate?: (password: string, isPassphrase: boolean) => void;
    onClose: () => void;
}


export default DeployEntityDialog;
