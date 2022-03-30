import React from 'react';

import Button from '@mui/material/Button';
import Checkbox from '@mui/material/Checkbox';
import Dialog from '@mui/material/Dialog';
import DialogActions from '@mui/material/DialogActions';
import DialogContent from '@mui/material/DialogContent';
import DialogContentText from '@mui/material/DialogContentText';
import DialogTitle from '@mui/material/DialogTitle';
import FormControlLabel from '@mui/material/FormControlLabel';
import IconButton from '@mui/material/IconButton';
import InputAdornment from '@mui/material/InputAdornment';
import TextField from '@mui/material/TextField';
import Tooltip from '@mui/material/Tooltip';

import ShowIcon from '@mui/icons-material/VisibilityOutlined';
import HideIcon from '@mui/icons-material/VisibilityOffOutlined';

import {useSelector, useDispatch} from '../../redux';
import {changePasswordVisibility, configureSsh, closeSshDialog} from '../../redux/ssh';


const DeployEntityDialog = (props: Props) => {
    const open = useSelector((state) => state.ssh.open);
    const show = useSelector((state) => state.ssh.show);
    const onValidate = useSelector((state) => state.ssh.nextAction);
    const dispatch = useDispatch();

    const [password, setPassword] = React.useState<string>("");
    const [isPassphrase, setIsPassphrase] = React.useState<boolean>(false);

    const changePassword = React.useCallback((event: React.ChangeEvent<HTMLInputElement>) => {
        setPassword(event.target.value);
    }, []);

    const changePassphrase = React.useCallback((event: React.ChangeEvent<HTMLInputElement>) => {
        setIsPassphrase(event.target.checked);
    }, []);

    const reset = React.useCallback(() => {
        setPassword("");
        setIsPassphrase(false);
    }, []);

    const handleClose = React.useCallback(() => {
        dispatch(closeSshDialog());
        reset();
    }, [reset, dispatch]);

    const handleValidate = React.useCallback((event: React.FormEvent<HTMLFormElement>) => {
        event.preventDefault();
        dispatch(configureSsh({password, isPassphrase}))
        onValidate && onValidate();
        handleClose();
    }, [dispatch, onValidate, handleClose, password, isPassphrase]);

    const handleVisibilityChange = React.useCallback(() => {
        dispatch(changePasswordVisibility());
    }, [dispatch]);

    return (
        <Dialog open={open} onClose={handleClose}>
            <form onSubmit={handleValidate}>
                <DialogTitle>Deploy an Entity</DialogTitle>
                <DialogContent>
                    <DialogContentText>Please specify the SSH password to upload files and/or connect to the entity</DialogContentText>
                    <FormControlLabel
                        control={<Checkbox checked={isPassphrase} onChange={changePassphrase} name="passphrase" />}
                        label="Passphrase for an SSH private key"
                    />
                    <TextField
                        autoFocus
                        margin="dense"
                        label={isPassphrase ? "Passphrase" : "Password"}
                        type={show ? undefined : "password"}
                        value={password}
                        onChange={changePassword}
                        fullWidth
                        InputProps={{
                            endAdornment: <InputAdornment position="end">
                                <Tooltip title={(show ? "Hide" : "Show") + " " + (isPassphrase ? "passphrase" : "password")} placement="top">
                                    <IconButton onClick={handleVisibilityChange}>{show ? <ShowIcon /> : <HideIcon />}</IconButton>
                                </Tooltip>
                            </InputAdornment>,
                        }}
                    />
                </DialogContent>
                <DialogActions>
                    <Button onClick={handleClose} color="primary">Cancel</Button>
                    <Button type="submit" color="primary">Configure</Button>
                </DialogActions>
            </form>
        </Dialog>
    );
};


interface Props {
}


export default DeployEntityDialog;
