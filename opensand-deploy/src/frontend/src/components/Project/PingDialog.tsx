import React from 'react';

import Button from '@material-ui/core/Button';
import Dialog from '@material-ui/core/Dialog';
import DialogActions from '@material-ui/core/DialogActions';
import DialogContent from '@material-ui/core/DialogContent';
import DialogContentText from '@material-ui/core/DialogContentText';
import DialogTitle from '@material-ui/core/DialogTitle';
import IconButton from '@material-ui/core/IconButton';
import InputAdornment from '@material-ui/core/InputAdornment';
import Menu from '@material-ui/core/Menu';
import MenuItem from '@material-ui/core/MenuItem';
import TextField from '@material-ui/core/TextField';

import OpenedIcon from '@material-ui/icons/ArrowDropUp';
import ClosedIcon from '@material-ui/icons/ArrowDropDown';


interface Props {
    open: boolean;
    entity?: string;
    destinations: string[];
    onValidate?: (destination: string) => void;
    onClose: () => void;
}


const PingDialog = (props: Props) => {
    const {open, entity, destinations, onValidate, onClose} = props;

    const [destinationsAnchor, setDestinationsAnchor] = React.useState<HTMLElement | null>(null);
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
        <Dialog open={open} onClose={handleClose}>
            <form onSubmit={handleValidate}>
                <DialogTitle>Ping from {entity ? `the entity ${entity}` : "an Entity"}</DialogTitle>
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


export default PingDialog;
