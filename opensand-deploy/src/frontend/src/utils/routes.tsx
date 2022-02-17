import React from 'react';
import {BrowserRouter as Router, Route, Redirect, Switch, RouteComponentProps} from 'react-router-dom';

import Editor from '../components/Editor/Editor';
import NotFound from '../components/common/NotFound';
import Project from '../components/Project/Project';
import Projects from '../components/HomePage/Projects';
import Template from '../components/common/Template';


const Routes = () => {
    const [tab, changeTab] = React.useState<number>(0);

    return (
        <Router>
            <Template>
                <Switch>
                    <Route exact path="/projects" component={Projects} />
                    <Route exact path="/project/:name" render={(props: RouteComponentProps<{name: string;}>) => <Project tab={tab} changeTab={changeTab} {...props} />} />
                    <Route exact path="/edit/:project/:model/:entity" component={Editor} />
                    <Route exact path="/edit/:project/:model" component={Editor} />
                    <Route exact path="/not-found" component={NotFound} />
                    <Redirect exact from="/" to="/projects" />
                    <Redirect from="*" to="/not-found" />
                </Switch>
            </Template>
        </Router>
    );
};


export default Routes;
