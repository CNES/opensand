import React from 'react';
import type {FormikProps} from 'formik';

import Button from '@mui/material/Button';

import {styled} from '@mui/material/styles';

import SingleFieldDialog from '../common//SingleFieldDialog';

import {updateXML} from '../../api';
import {useDispatch} from '../../redux';
import {useOpen, useFormSubmit} from '../../utils/hooks';
import type {Component} from '../../xsd';


export const SpacedButton = styled(Button, {name: "SpacedButton", slot: "Wrapper"})(({ theme }) => ({
    marginRight: theme.spacing(2),
}));


const SaveAsButton: React.FC<Props> = (props) => {
    const {project, xsd, form: {values, setSubmitting, isSubmitting}} = props;

    const dispatch = useDispatch();
    const onSubmitted = useFormSubmit();

    const [open, handleOpen, handleClose] = useOpen();

    const handleSave = React.useCallback((template: string) => {
        handleClose();
        setSubmitting(true);
        const url = "template/" + xsd + "/" + template;
        dispatch(updateXML({project, xsd, urlFragment: url, root: values}));
        onSubmitted(() => () => setSubmitting(false));
    }, [dispatch, project, xsd, values, onSubmitted, setSubmitting, handleClose]);

    return (
        <React.Fragment>
            <SpacedButton
                disabled={xsd == null || isSubmitting}
                color="secondary"
                variant="contained"
                onClick={handleOpen}
            >
                Save As Template
            </SpacedButton>
            <SingleFieldDialog
                open={open}
                title="Save Template"
                description="Please enter the name to save the template as."
                fieldLabel="Template Name"
                onValidate={handleSave}
                onClose={handleClose}
                validateButtonLabel="Save"
            />
        </React.Fragment>
    );
};


interface Props {
    project: string;
    xsd: string;
    form: FormikProps<Component>;
}


export default SaveAsButton;
