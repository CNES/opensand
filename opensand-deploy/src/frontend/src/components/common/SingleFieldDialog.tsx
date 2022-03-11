import React from 'react';

import Button from '@mui/material/Button';
import Dialog from '@mui/material/Dialog';
import DialogActions from '@mui/material/DialogActions';
import DialogContent from '@mui/material/DialogContent';
import DialogContentText from '@mui/material/DialogContentText';
import DialogTitle from '@mui/material/DialogTitle';
import TextField from '@mui/material/TextField';


const SingleFieldDialog: React.FC<Props> = (props) => {
    const {open, title, description, fieldLabel, onValidate, onClose} = props;
    const okLabel = props.validateButtonLabel != null ? props.validateButtonLabel : "Create";
    const koLabel = props.closeButtonLabel != null ? props.closeButtonLabel : "Cancel";

    const [value, setValue] = React.useState<string>("");

    const changeValue = React.useCallback((event: React.ChangeEvent<HTMLInputElement>) => {
        setValue(event.target.value);
    }, []);

    const handleValidate = React.useCallback((event: React.FormEvent<HTMLFormElement>) => {
        event.preventDefault();
        onValidate(value);
        setValue("");
    }, [onValidate, value])

    return (
        <Dialog open={open} onClose={onClose}>
            <form onSubmit={handleValidate}>
                <DialogTitle>{title}</DialogTitle>
                <DialogContent>
                    <DialogContentText>{description}</DialogContentText>
                    <TextField
                        autoFocus
                        margin="dense"
                        label={fieldLabel}
                        value={value}
                        onChange={changeValue}
                        fullWidth
                    />
                </DialogContent>
                <DialogActions>
                    <Button onClick={onClose} color="primary">{koLabel}</Button>
                    <Button type="submit" color="primary">{okLabel}</Button>
                </DialogActions>
            </form>
        </Dialog>
    );
};


interface Props {
    open: boolean;
    title: string;
    description: string;
    fieldLabel: string;
    onValidate: (value: string) => void;
    onClose: () => void;
    validateButtonLabel?: string;
    closeButtonLabel?: string;
}


export default SingleFieldDialog;
