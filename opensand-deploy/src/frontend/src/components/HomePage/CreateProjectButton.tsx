import React from 'react';
import {useHistory} from 'react-router-dom';

import Button from '@material-ui/core/Button';

import {getProjectModel, updateProject, IApiSuccess, IXsdContent} from '../../api';
import {fromXSD} from '../../xsd/parser';
import {Component, Parameter} from '../../xsd/model';

import SingleFieldDialog from '../common/SingleFieldDialog';


const CreateProjectButton = () => {
    const [open, setOpen] = React.useState<boolean>(false);
    const history = useHistory();

    const handleOpen = React.useCallback(() => {
        setOpen(true);
    }, [setOpen]);

    const handleClose = React.useCallback(() => {
        setOpen(false);
    }, [setOpen]);

    const handleCreatedProject = React.useCallback((projectName: string, success: IApiSuccess) => {
        console.log(success);
        if (success.status === "OK") {
            setOpen(false);
            history.push("/project/" + projectName);
        }
    }, [setOpen, history]);

    const fillProjectModel = React.useCallback((projectName: string, content: IXsdContent) => {
        const model = fromXSD(content.content);
        model.root.children.forEach((c: Component) => {
            if (c.id === "project") {
                c.parameters.forEach((p: Parameter) => {
                    if (p.id === "name") {
                        p.value = projectName;
                    }
                });
            }
        });
        updateProject(handleCreatedProject.bind(this, projectName), console.log, projectName, model);
    }, [handleCreatedProject]);

    const doCreateProject = React.useCallback((projectName: string) => {
        if (projectName !== "") {
            getProjectModel(fillProjectModel.bind(this, projectName), console.log);
        }
    }, [fillProjectModel]);

    return (
        <React.Fragment>
            <Button color="primary" onClick={handleOpen}>Create</Button>
            <SingleFieldDialog
                open={open}
                title="New Project"
                description="Please enter the name of your new project."
                fieldLabel="Project Name"
                onValidate={doCreateProject}
                onClose={handleClose}
            />
        </React.Fragment>
    );
};


export default CreateProjectButton;
