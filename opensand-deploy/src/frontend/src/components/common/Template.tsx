import React from 'react';
import {useHistory} from 'react-router-dom';

import AppBar from '@material-ui/core/AppBar';
import IconButton from '@material-ui/core/IconButton';
import Paper from '@material-ui/core/Paper';
import Toolbar from '@material-ui/core/Toolbar';
import Typography from '@material-ui/core/Typography';

import {makeStyles, Theme} from '@material-ui/core/styles';
import BackIcon from '@material-ui/icons/ArrowBackIos';

import {registerCallback, unregisterCallback} from '../../utils/dispatcher';

import Logger from './Logger';


interface Props {
    children?: React.ReactNode;
}


interface ErrorMessage {
    message?: string;
    date: Date;
}


const useStyles = makeStyles((theme: Theme) => ({
    header: {
        flex: "0 1 auto",
    },
    content: {
        flex: "1 1 auto",
    },
    expand: {
        flexGrow: 1,
    },
}));


const Template = (props: Props) => {
    const [error, changeError] = React.useState<ErrorMessage>({date: new Date()});
    const history = useHistory();
    const classes = useStyles();

    const newError = React.useCallback((message: string) => {
        changeError({message, date: new Date()});
    }, [changeError]);

    const showHistory = React.useCallback(() => {
        const [, place, project] = history.location.pathname.split('/');
        if (place === "edit") {
            history.push(`/project/${project}`);
        } else {
            history.push("/projects");
        }
    }, [history]);

    React.useEffect(() => {
        if (!registerCallback("errorCallback", newError)) {
            newError("Failed registering error callback: further errors will not display here.");
        }
        return () => {unregisterCallback("errorCallback");}
    }, [newError]);

    return (
        <React.Fragment>
            <AppBar className={classes.header} position="static">
                <Toolbar>
                    <IconButton edge="start" color="inherit" onClick={showHistory}>
                        <BackIcon />
                    </IconButton>
                    <Typography variant="h6" className={classes.expand}>
                        OpenSAND Conf 6.0.0
                    </Typography>
                    <Logger message={error.message} date={error.date} />
                </Toolbar>
            </AppBar>
            <Paper elevation={0} className={classes.content}>
                {props.children}
            </Paper>
        </React.Fragment>
    );
};


export default Template;
