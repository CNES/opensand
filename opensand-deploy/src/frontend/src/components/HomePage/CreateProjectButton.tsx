import React from 'react';
import {useHistory} from 'react-router-dom';

import Button from '@material-ui/core/Button';

import {getProjectModel, updateProject, IApiSuccess, IXsdContent} from '../../api';
import {sendError} from '../../utils/dispatcher';
import {fromXSD} from '../../xsd/parser';

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
        if (success.status === "OK") {
            setOpen(false);
            history.push("/project/" + projectName);
        }
    }, [setOpen, history]);

    const fillProjectModel = React.useCallback((projectName: string, content: IXsdContent) => {
        const model = fromXSD(content.content);
        model.root.elements.forEach((e) => {
            if (e.type === "component" && e.element.id === "platform") {
                e.element.elements.forEach((p) => {
                    if (p.type === "parameter" && p.element.id === "project") {
                        p.element.value = projectName;
                    }
                });
            }
        });
        const onSuccess = handleCreatedProject.bind(this, projectName);
        updateProject(onSuccess, sendError, projectName, model);
    }, [handleCreatedProject]);

    const doCreateProject = React.useCallback((projectName: string) => {
        if (projectName !== "") {
            getProjectModel(fillProjectModel.bind(this, projectName), sendError);
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
