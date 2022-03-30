import React from 'react';
import {BrowserRouter as Router, Routes, Route, Navigate} from 'react-router-dom';

import Editor from '../components/Editor/Editor';
import NotFound from '../components/common/NotFound';
import Project from '../components/Project/Project';
import Projects from '../components/HomePage/Projects';
import Template from '../components/common/Template';


const SiteMap = () => {
    return (
        <Router>
            <Template>
                <Routes>
                    <Route path="/projects" element={<Projects />} />
                    <Route path="/project/:name" element={<Project />} />
                    <Route path="/edit/:project/:model/:entity" element={<Editor />} />
                    <Route path="/edit/:project/:model" element={<Editor />} />
                    <Route path="/not-found" element={<NotFound />} />
                    <Route path="/" element={<Navigate to="/projects" replace />} />
                    <Route path="*" element={<Navigate to="/not-found" replace />} />
                </Routes>
            </Template>
        </Router>
    );
};


export default SiteMap;
