import React from 'react';
import {Formik} from 'formik';
import type {FormikProps, FormikHelpers} from 'formik';

import Button from '@mui/material/Button';
import Dialog from '@mui/material/Dialog';
import DialogActions from '@mui/material/DialogActions';
import DialogContent from '@mui/material/DialogContent';
import DialogContentText from '@mui/material/DialogContentText';
import DialogTitle from '@mui/material/DialogTitle';

import Parameter from '../Model/Parameter';

import {noActions} from '../../utils/actions';
import type {Parameter as ParameterType} from '../../xsd';


interface Values {
    name: ParameterType;
    type: ParameterType;
}


const NewEntityDialog = (props: Props) => {
    const {open, entityName, entityType, onValidate, onClose} = props;

    const autoSave = React.useMemo(() => () => {}, []);

    const nameParam = React.useMemo(() => {
        const cloned = {...entityName};
        cloned.readOnly = false;
        cloned.value = "";
        return cloned;
    }, [entityName]);

    const typeParam = React.useMemo(() => {
        const cloned = {...entityType};
        cloned.readOnly = false;
        cloned.value = "";
        return cloned;
    }, [entityType]);

    const initialValues: Values = React.useMemo(() => ({
        name: nameParam,
        type: typeParam,
    }), [nameParam, typeParam]);

    const handleClose = React.useCallback(() => {
        onClose();
    }, [onClose]);

    const handleSubmit = React.useCallback((values: Values, helpers: FormikHelpers<Values>) => {
        onValidate(values.name.value, values.type.value);
        handleClose();
    }, [onValidate, handleClose]);

    return (
        <Dialog open={open} onClose={handleClose}>
            <Formik initialValues={initialValues} onSubmit={handleSubmit}>
                {(formik: FormikProps<Values>) => (
                    <form onSubmit={formik.handleSubmit}>
                        <DialogTitle>Add a new Entity in your Platform</DialogTitle>
                        <DialogContent>
                            <DialogContentText>
                                Please give a name and select the role of your machine.
                            </DialogContentText>
                            <Parameter parameter={formik.values.name} prefix="name" form={formik} actions={noActions} autoSave={autoSave} />
                            <Parameter parameter={formik.values.type} prefix="type" form={formik} actions={noActions} autoSave={autoSave} />
                        </DialogContent>
                        <DialogActions>
                            <Button onClick={handleClose} color="primary">Cancel</Button>
                            <Button type="submit" color="primary">Add Entity</Button>
                        </DialogActions>
                    </form>
                )}
            </Formik>
        </Dialog>
    );
};


interface Props {
    open: boolean;
    entityName: ParameterType;
    entityType: ParameterType;
    onValidate: (entity: string, entityType: string) => void;
    onClose: () => void;
}


export default NewEntityDialog;
