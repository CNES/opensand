import React, {useMemo} from 'react';
import ReactDOM from 'react-dom';

import useMediaQuery from '@material-ui/core/useMediaQuery';
import {ThemeProvider} from '@material-ui/core/styles';

import './index.css';
import createTheme from './utils/theme';
import Routes from './utils/routes';

import reportWebVitals from './reportWebVitals';


function App() {
    const prefersDarkMode = useMediaQuery("(prefers-color-scheme: dark)");
    const theme = useMemo(() => createTheme(prefersDarkMode), [prefersDarkMode]);

    return (
        <React.StrictMode>
            <ThemeProvider theme={theme}>
                <Routes />
            </ThemeProvider>
        </React.StrictMode>
    );
}


ReactDOM.render(<App />, document.getElementById('root'));

// If you want to start measuring performance in your app, pass a function
// to log results (for example: reportWebVitals(console.log))
// or send to an analytics endpoint. Learn more: https://bit.ly/CRA-vitals
reportWebVitals(console.log);
