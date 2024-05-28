import React from 'react';

import Badge from '@mui/material/Badge';
import Button from '@mui/material/Button';
import Box from '@mui/material/Box';
import Divider from '@mui/material/Divider';
import IconButton from '@mui/material/IconButton';
import Popover from '@mui/material/Popover';

import LogIcon from '@mui/icons-material/Announcement';

import {useSelector, useDispatch} from '../../redux';
import {removeError, clearErrors, clearNotifications} from '../../redux/error';


const Logger: React.FC<Props> = (props) => {
    const messages = useSelector((state) => state.error.messages);
    const unread = useSelector((state) => state.error.unread);
    const dispatch = useDispatch();

    const [anchor, setAnchor] = React.useState<HTMLButtonElement | null>(null);

    const handleClick = React.useCallback((event: React.MouseEvent<HTMLButtonElement>) => {
        dispatch(clearNotifications());
        setAnchor(event.currentTarget);
    }, [dispatch]);

    const handleClose = React.useCallback(() => {
        setAnchor(null);
    }, []);

    const handleClear = React.useCallback(() => {
        dispatch(clearErrors());
        setAnchor(null);
    }, [dispatch]);

    const handleRemove = React.useCallback((index: number) => {
        dispatch(removeError(index));
    }, [dispatch]);

    const headers = [
        <Button key={0} onClick={handleClear}>Clear Messages</Button>,
        <Divider key={1} />,
    ];

    return (
        <React.Fragment>
            <IconButton color="inherit" onClick={handleClick}>
                <Badge badgeContent={unread} color="secondary">
                    <LogIcon />
                </Badge>
            </IconButton>
            <Popover
                open={Boolean(anchor)}
                anchorEl={anchor}
                onClose={handleClose}
                anchorOrigin={{
                    vertical: "bottom",
                    horizontal: "right",
                }}
                transformOrigin={{
                    vertical: "top",
                    horizontal: "right",
                }}
            >
                <Box width="300px" p={2}>
                    {headers.concat(...messages.map((message: string, i: number) => [
                        <Box
                            key={2*i+headers.length}
                            component="pre"
                            sx={{whiteSpace: "pre-wrap"}}
                            onClick={() => handleRemove(i)}
                        >
                            {message}
                        </Box>,
                        <Divider key={2*i+headers.length+1}/>,
                    ]))}
                </Box>
            </Popover>
        </React.Fragment>
    );
};


interface Props {
}


export default Logger;
