import React from 'react';

import Button from '@material-ui/core/Button';
import Checkbox from '@material-ui/core/Checkbox';
import Dialog from '@material-ui/core/Dialog';
import DialogActions from '@material-ui/core/DialogActions';
import DialogContent from '@material-ui/core/DialogContent';
import DialogContentText from '@material-ui/core/DialogContentText';
import DialogTitle from '@material-ui/core/DialogTitle';
import FormControlLabel from '@material-ui/core/FormControlLabel';
import TextField from '@material-ui/core/TextField';


interface Props {
    open: boolean;
    onValidate?: (password: string, isPassphrase: boolean) => void;
    onClose: () => void;
}


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


export default DeployEntityDialog;
