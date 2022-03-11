import {createTheme, Theme} from '@mui/material/styles';
import {blue} from '@mui/material/colors';


export const createOpensandTheme = (prefersDarkMode: boolean): Theme => createTheme({
    palette: {
        mode: prefersDarkMode ? 'dark' : 'light',
        primary: {
            main: "#00BCD4",
        },
        secondary: {
            main: "#FFAE42",
        },
    },
    components: {
        MuiAccordionSummary: {
            styleOverrides: {
                root: {
                    backgroundColor: blue[200],
                    '&.Mui-expanded': {
                        backgroundColor: blue[400],
                    },
                },
            },
        },
        MuiTooltip: {
            styleOverrides: {
                tooltip: {
                    fontSize: "1em",
                },
            },
        },
        MuiToolbar: {
            styleOverrides: {
                root: {
                    backgroundColor: blue[700],
                },
            },
        },
    },
});


export default createOpensandTheme;
