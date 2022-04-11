import React from 'react';

import Button from '@mui/material/Button';
import Dialog from '@mui/material/Dialog';
import DialogActions from '@mui/material/DialogActions';
import DialogContent from '@mui/material/DialogContent';
import DialogContentText from '@mui/material/DialogContentText';
import DialogTitle from '@mui/material/DialogTitle';
import IconButton from '@mui/material/IconButton';
import InputAdornment from '@mui/material/InputAdornment';
import Menu from '@mui/material/Menu';
import MenuItem from '@mui/material/MenuItem';
import TextField from '@mui/material/TextField';

import OpenedIcon from '@mui/icons-material/ArrowDropUp';
import ClosedIcon from '@mui/icons-material/ArrowDropDown';

import {useSelector, useDispatch} from '../../redux';
import {clearPing, closePing} from '../../redux/ping';


const PingDialog: React.FC<Props> = (props) => {
    const {onValidate} = props;

    const destinations = useSelector((state) => state.ping.destinations);
    const entity = useSelector((state) => state.ping.source);
    const open = useSelector((state) => state.ping.status);
    const dispatch = useDispatch();

    const [destinationsAnchor, setDestinationsAnchor] = React.useState<HTMLElement | null>(null);
    const [destination, setDestination] = React.useState<string>("");

    const changeDestination = React.useCallback((event: React.ChangeEvent<HTMLInputElement>) => {
        setDestination(event.target.value);
    }, []);

    const handleClose = React.useCallback(() => {
        setDestination("");
        dispatch(clearPing());
    }, [dispatch]);

    const handleValidate = React.useCallback((event: React.FormEvent<HTMLFormElement>) => {
        event.preventDefault();
        onValidate(destination);
        setDestination("");
        dispatch(closePing());
    }, [onValidate, handleClose, destination]);

    const handleCloseDestinations = React.useCallback(() => {
        setDestinationsAnchor(null);
    }, []);

    const handleShowDestinations = React.useCallback((event: React.MouseEvent<HTMLButtonElement>) => {
        setDestinationsAnchor(event.currentTarget);
    }, []);

    const handleSelectDestination = React.useCallback((dest: string) => {
        setDestination(dest);
        setDestinationsAnchor(null);
    }, []);

    const handleMouseDownPrevent = React.useCallback((event: React.MouseEvent<HTMLButtonElement>) => {
        event.preventDefault();
    }, []);

    const selectedIndex = destinations.findIndex((dest: string) => dest === destination);

    return (
        <Dialog open={open === "loading" || open === "selection"} onClose={handleClose}>
            <form onSubmit={handleValidate}>
                <DialogTitle>Ping from {entity ? `the entity ${entity.name}` : "an Entity"}</DialogTitle>
                <DialogContent>
                    <DialogContentText>Please specify the destination of the ping command</DialogContentText>
                    <TextField
                        autoFocus
                        margin="dense"
                        label="Destination"
                        value={destination}
                        onChange={changeDestination}
                        fullWidth
                        InputProps={{endAdornment: (
                            <InputAdornment position="end">
                                <IconButton
                                  onClick={handleShowDestinations}
                                  onMouseDown={handleMouseDownPrevent}
                                >
                                    {destinationsAnchor ? <OpenedIcon /> : <ClosedIcon />}
                                </IconButton>
                            </InputAdornment>
                        )}}
                    />
                    <Menu
                        keepMounted
                        anchorEl={destinationsAnchor}
                        open={Boolean(destinationsAnchor)}
                        onClose={handleCloseDestinations}
                    >
                        {destinations.map((dest: string, index: number) => (
                            <MenuItem
                                key={index}
                                selected={index === selectedIndex}
                                onClick={() => handleSelectDestination(dest)}
                            >
                                {dest}
                            </MenuItem>
                        ))}
                    </Menu>
                </DialogContent>
                <DialogActions>
                    <Button onClick={handleClose} color="primary">Cancel</Button>
                    <Button type="submit" color="primary">Ping</Button>
                </DialogActions>
            </form>
        </Dialog>
    );
};


interface Props {
    onValidate: (destination: string) => void;
}


export default PingDialog;
