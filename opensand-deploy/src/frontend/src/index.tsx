import React from 'react';
import ReactDOM from 'react-dom';
import {Provider} from 'react-redux';

import useMediaQuery from '@mui/material/useMediaQuery';
import {ThemeProvider} from '@mui/material/styles';

import './index.css';
import store from './redux';
import createTheme from './utils/theme';
import Routes from './utils/routes';


function App() {
    const prefersDarkMode = useMediaQuery("(prefers-color-scheme: dark)");
    const theme = React.useMemo(() => createTheme(prefersDarkMode), [prefersDarkMode]);

    return (
        <React.StrictMode>
            <Provider store={store}>
                <ThemeProvider theme={theme}>
                    <Routes />
                </ThemeProvider>
            </Provider>
        </React.StrictMode>
    );
}


ReactDOM.render(<App />, document.getElementById('root'));
