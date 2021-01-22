import {createMuiTheme, Theme} from '@material-ui/core/styles';
import {blue} from '@material-ui/core/colors';


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
                // backgroundColor: "rgba(255,215,0,0.2)",
                backgroundColor: blue[200],
                '&$expanded': {
                    // backgroundColor: "rgba(255,215,0,0.5)",
                    backgroundColor: blue[400],
                },
            },
        },
        MuiAppBar: {
            colorPrimary: {
                // backgroundColor: "rgba(255,215,0,0.5)",
                backgroundColor: "#FFFACD",
            },
        },
    },
});


export default createTheme;
