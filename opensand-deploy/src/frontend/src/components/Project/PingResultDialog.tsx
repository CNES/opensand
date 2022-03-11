import React from 'react';

import Button from '@mui/material/Button';
import CircularProgress from '@mui/material/CircularProgress';
import Dialog from '@mui/material/Dialog';
import DialogActions from '@mui/material/DialogActions';
import DialogContent from '@mui/material/DialogContent';
import DialogContentText from '@mui/material/DialogContentText';
import DialogTitle from '@mui/material/DialogTitle';

import {useSelector, useDispatch} from '../../redux';
import {clearPing} from '../../redux/ping';


const PingResultDialog: React.FC<Props> = (props) => {
    const content = useSelector((state) => state.ping.result);
    const open = useSelector((state) => state.ping.status);
    const dispatch = useDispatch();

    const handleClose = React.useCallback(() => {
        dispatch(clearPing());
    }, [dispatch]);

    const message = Boolean(content) ? <pre>{content}</pre> : <CircularProgress />;

    return (
        <Dialog open={open === "pending" || open === "success"} onClose={handleClose}>
            <DialogTitle>Ping Results</DialogTitle>
            <DialogContent>
                <DialogContentText>{message}</DialogContentText>
            </DialogContent>
            <DialogActions>
                <Button onClick={handleClose} color="primary">OK</Button>
            </DialogActions>
        </Dialog>
    );
};


interface Props {
}


export default PingResultDialog;
