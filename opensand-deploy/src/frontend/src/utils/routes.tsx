import React from 'react';
import {BrowserRouter as Router, Route, Redirect, Switch} from 'react-router-dom';

import App from '../components/App';


const Routes = () => (
    <Router>
        <Switch>
            <Route exact path="/" component={App} />
            <Redirect from="*" to="/" />
        </Switch>
    </Router>
);


export default Routes;
