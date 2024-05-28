import React from 'react';
import {useLocation, useNavigate} from 'react-router-dom';

import AppBar from '@mui/material/AppBar';
import IconButton from '@mui/material/IconButton';
import Paper from '@mui/material/Paper';
import Toolbar from '@mui/material/Toolbar';
import Typography from '@mui/material/Typography';

import BackIcon from '@mui/icons-material/ArrowBackIos';

import Logger from './Logger';


const opensandVersion = process.env.REACT_APP_OPENSAND_VERSION || `(${process.env.NODE_ENV}  build)`;


const Template: React.FC<React.PropsWithChildren<Props>> = (props) => {
    const {pathname} = useLocation();
    const navigate = useNavigate();

    const navigateUp = React.useCallback(() => {
        const [, place, project] = pathname.split('/');
        if (place === "edit") {
            navigate(`/project/${project}`);
        } else {
            navigate("/projects");
        }
    }, [navigate, pathname]);

    return (
        <React.Fragment>
            <AppBar position="static" sx={{flex: "0 1 auto"}}>
                <Toolbar>
                    <IconButton edge="start" color="inherit" onClick={navigateUp}>
                        <BackIcon />
                    </IconButton>
                    <Typography variant="h6" sx={{flexGrow: 1}}>
                        OpenSAND Conf {opensandVersion}
                    </Typography>
                    <Logger />
                </Toolbar>
            </AppBar>
            <Paper elevation={0} sx={{flex: "1 1 auto"}}>
                {props.children}
            </Paper>
        </React.Fragment>
    );
};


interface Props {
}


export default Template;
