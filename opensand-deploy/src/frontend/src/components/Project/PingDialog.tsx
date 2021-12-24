import React from 'react';

import Button from '@material-ui/core/Button';
import Dialog from '@material-ui/core/Dialog';
import DialogActions from '@material-ui/core/DialogActions';
import DialogContent from '@material-ui/core/DialogContent';
import DialogContentText from '@material-ui/core/DialogContentText';
import DialogTitle from '@material-ui/core/DialogTitle';
import TextField from '@material-ui/core/TextField';


interface Props {
    open: boolean;
    destinations: string[];
    onValidate?: (destination: string) => void;
    onClose: () => void;
}


const PingDialog = (props: Props) => {
    const {open, onValidate, onClose} = props;

    const [destination, setDestination] = React.useState<string>("");

    const changeDestination = React.useCallback((event: React.ChangeEvent<HTMLInputElement>) => {
        setDestination(event.target.value);
    }, []);

    const handleClose = React.useCallback(() => {
        onClose();
        setDestination("");
    }, [onClose]);

    const handleValidate = React.useCallback((event: React.FormEvent<HTMLFormElement>) => {
        event.preventDefault();
        onValidate && onValidate(destination);
        handleClose();
    }, [onValidate, handleClose, destination]);

    return (
        <Dialog open={open} onClose={handleClose}>
            <form onSubmit={handleValidate}>
                <DialogTitle>Ping from an Entity</DialogTitle>
                <DialogContent>
                    <DialogContentText>Please specify the destination of the ping command</DialogContentText>
                    <TextField
                        autoFocus
                        margin="dense"
                        label="Destination"
                        value={destination}
                        onChange={changeDestination}
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


export default PingDialog;
