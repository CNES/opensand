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
    title: string;
    description: string;
    fieldLabel: string;
    onValidate: (value: string) => void;
    onClose: () => void;
    validateButtonLabel?: string;
    closeButtonLabel?: string;
}


const SingleFieldDialog = (props: Props) => {
    const {open, title, description, fieldLabel, onValidate, onClose} = props;
    const okLabel = props.validateButtonLabel != null ? props.validateButtonLabel : "Create";
    const koLabel = props.closeButtonLabel != null ? props.closeButtonLabel : "Cancel";

    const [value, setValue] = React.useState<string>("");

    const changeValue = React.useCallback((event: React.ChangeEvent<HTMLInputElement>) => {
        setValue(event.target.value);
    }, [setValue]);

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


export default SingleFieldDialog;
