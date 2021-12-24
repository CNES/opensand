import React from 'react';

import Button from '@material-ui/core/Button';
import Dialog from '@material-ui/core/Dialog';
import DialogActions from '@material-ui/core/DialogActions';
import DialogContent from '@material-ui/core/DialogContent';
import DialogContentText from '@material-ui/core/DialogContentText';
import DialogTitle from '@material-ui/core/DialogTitle';


interface Props {
    open: boolean;
    content?: string;
    onClose: () => void;
}


const PingResultDialog = (props: Props) => {
    const {open, content, onClose} = props;

    return (
        <Dialog open={open} onClose={onClose}>
            <DialogTitle>Ping Results</DialogTitle>
            <DialogContent>
                <DialogContentText>{content}</DialogContentText>
            </DialogContent>
            <DialogActions>
                <Button onClick={onClose} color="primary">OK</Button>
            </DialogActions>
        </Dialog>
    );
};


export default PingResultDialog;
