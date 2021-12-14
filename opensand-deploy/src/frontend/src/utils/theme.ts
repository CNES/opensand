import {unstable_createMuiStrictModeTheme as createMuiTheme, makeStyles, Theme} from '@material-ui/core/styles';
import {blue} from '@material-ui/core/colors';


export const componentStyles = makeStyles((theme: Theme) => ({
    root: {
        width: "98%",
        marginLeft: "1%",
        marginRight: "1%",
        marginTop: theme.spacing(1),
    },
    heading: {
        fontSize: theme.typography.pxToRem(15),
        flexBasis: "33.33%",
        flexShrink: 0,
    },
    secondaryHeading: {
        fontSize: theme.typography.pxToRem(15),
        color: theme.palette.text.secondary,
    },
}));


export const parameterStyles = makeStyles((theme: Theme) => ({
    fullWidth: {
        width: "100%",
        flexGrow: 1,
    },
    spaced: {
        display: "flex",
        marginBottom: theme.spacing(1),
    },
}));


export const createTheme = (prefersDarkMode: boolean): Theme => createMuiTheme({
    palette: {
        type: prefersDarkMode ? 'dark' : 'light',
        primary: {
            main: "#00BCD4",
        },
    },
    overrides: {
        MuiTooltip: {
            tooltip: {
                fontSize: "1em",
            },
        },
        MuiToolbar: {
            root: {
                backgroundColor: blue[700],
            },
        },
        MuiAccordionSummary: {
            root: {
                backgroundColor: blue[200],
                '&$expanded': {
                    backgroundColor: blue[400],
                },
            },
        },
        MuiAppBar: {
            colorPrimary: {
                backgroundColor: "#FFFACD",
            },
        },
    },
});


export default createTheme;
