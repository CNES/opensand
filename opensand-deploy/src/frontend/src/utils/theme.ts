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
