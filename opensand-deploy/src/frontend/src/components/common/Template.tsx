import React from 'react';

import AppBar from '@material-ui/core/AppBar';
import Badge from '@material-ui/core/Badge';
import IconButton from '@material-ui/core/IconButton';
import Paper from '@material-ui/core/Paper';
import Toolbar from '@material-ui/core/Toolbar';
import Typography from '@material-ui/core/Typography';

import LogIcon from '@material-ui/icons/Announcement';

import {makeStyles, Theme} from '@material-ui/core/styles';


interface Props {
    children?: React.ReactNode;
}


const useStyles = makeStyles((theme: Theme) => ({
    header: {
        flex: "0 1 auto",
    },
    content: {
        flex: "1 1 auto",
    },
/*
    footer: {
        flex: "0 1 " + theme.spacing(5),
    },
*/
    expand: {
        flexGrow: 1,
    },
}));


const Template = (props: Props) => {
    const classes = useStyles();

    return (
        <React.Fragment>
            <AppBar className={classes.header} position="static">
                <Toolbar>
                    <Typography variant="h6" className={classes.expand}>
                        OpenSAND Conf 6.0.0
                    </Typography>
                    <IconButton color="inherit">
                        <Badge badgeContent={4} color="secondary">
                            <LogIcon />
                        </Badge>
                    </IconButton>
                </Toolbar>
            </AppBar>
            <Paper elevation={0} className={classes.content}>
                {props.children}
            </Paper>
        </React.Fragment>
    );
};


export default Template;
