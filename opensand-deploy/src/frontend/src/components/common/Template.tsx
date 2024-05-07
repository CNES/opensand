import React from 'react';
import {useLocation, useNavigate} from 'react-router-dom';

import AppBar from '@mui/material/AppBar';
import IconButton from '@mui/material/IconButton';
import Paper from '@mui/material/Paper';
import Toolbar from '@mui/material/Toolbar';

import {styled} from '@mui/material/styles';
import BackIcon from '@mui/icons-material/ArrowBackIos';

import Logger from './Logger';
import GrowingTypography from './GrowingTypography';


const FlexAppBar = styled(AppBar, {name: "FlexAppBar", slot: "Wrapper"})({
    flex: "0 1 auto",
});


const FlexPaper = styled(Paper, {name: "FlexPaper", slot: "Wrapper"})({
    flex: "1 1 auto",
});


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
            <FlexAppBar position="static">
                <Toolbar>
                    <IconButton edge="start" color="inherit" onClick={navigateUp}>
                        <BackIcon />
                    </IconButton>
                    <GrowingTypography variant="h6">
                        OpenSAND Conf {opensandVersion}
                    </GrowingTypography>
                    <Logger />
                </Toolbar>
            </FlexAppBar>
            <FlexPaper elevation={0}>
                {props.children}
            </FlexPaper>
        </React.Fragment>
    );
};


interface Props {
}


export default Template;
