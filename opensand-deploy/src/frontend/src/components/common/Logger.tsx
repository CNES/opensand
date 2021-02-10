import React from 'react';

import Badge from '@material-ui/core/Badge';
import Button from '@material-ui/core/Button';
import Box from '@material-ui/core/Box';
import Divider from '@material-ui/core/Divider';
import IconButton from '@material-ui/core/IconButton';
import Popover from '@material-ui/core/Popover';
import Typography from '@material-ui/core/Typography';

import LogIcon from '@material-ui/icons/Announcement';


interface Props {
    date: Date;
    message?: string;
}


interface State {
    unreadNotifications: number;
    messages: string[];
}


interface Action {
    addMessage?: string;
    removeMessage?: number;
    clearMessages?: boolean;
    clearNotifications?: boolean;
}


const reducer = (state: State, action: Action): State => {
    const {addMessage, removeMessage, clearMessages, clearNotifications} = action;

    if (addMessage != null) {
        return {
            unreadNotifications: state.unreadNotifications + 1,
            messages: [addMessage, ...state.messages],
        };
    } else if (removeMessage != null) {
        state.messages.splice(removeMessage, 1);
        return state;
    } else if (clearMessages) {
        return {unreadNotifications: 0, messages: []};
    } else if (clearNotifications) {
        return {...state, unreadNotifications: 0};
    }

    return state;
};


const INITIAL_STATE: State = {
    messages: [],
    unreadNotifications: 0,
};


const Logger = (props: Props) => {
    const [state, dispatch] = React.useReducer(reducer, INITIAL_STATE);
    const [anchor, setAnchor] = React.useState<HTMLButtonElement | null>(null);

    const handleClick = React.useCallback((event: React.MouseEvent<HTMLButtonElement>) => {
        dispatch({clearNotifications: true});
        setAnchor(event.currentTarget);
    }, [setAnchor, dispatch]);

    const handleClose = React.useCallback(() => {
        setAnchor(null);
    }, [setAnchor]);

    const handleClear = React.useCallback(() => {
        dispatch({clearMessages: true});
        setAnchor(null);
    }, [dispatch, setAnchor]);

    const handleRemove = React.useCallback((index: number) => {
        dispatch({removeMessage: index});
    }, [dispatch]);

    React.useEffect(() => {
        dispatch({addMessage: props.message});
    }, [props.date, props.message]);

    const headers = [
        <Button key={0} onClick={handleClear}>Clear Messages</Button>,
        <Divider key={1} />,
    ];

    return (
        <React.Fragment>
            <IconButton color="inherit" onClick={handleClick}>
                <Badge badgeContent={state.unreadNotifications} color="secondary">
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
                    {headers.concat(...state.messages.map((message: string, i: number) => [
                        <Typography key={2*i+headers.length} variant="body1" onClick={() => handleRemove(i)}>
                            {message}
                        </Typography>,
                        <Divider key={2*i+headers.length+1}/>,
                    ]))}
                </Box>
            </Popover>
        </React.Fragment>
    );
};


export default Logger;
