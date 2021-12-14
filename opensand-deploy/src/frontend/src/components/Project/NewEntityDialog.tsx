import React from 'react';

import Button from '@material-ui/core/Button';
import Dialog from '@material-ui/core/Dialog';
import DialogActions from '@material-ui/core/DialogActions';
import DialogContent from '@material-ui/core/DialogContent';
import DialogContentText from '@material-ui/core/DialogContentText';
import DialogTitle from '@material-ui/core/DialogTitle';

import {noActions} from '../../utils/actions';
import {Parameter as ParameterType} from '../../xsd/model';

import Parameter from '../Model/Parameter';


interface Props {
    open: boolean;
    entityName: ParameterType;
    entityType: ParameterType;
    onValidate: (entity: string, entityType: string) => void;
    onClose: () => void;
}


const NewEntityDialog = (props: Props) => {
    const {open, entityName, entityType, onValidate, onClose} = props;

    const [entity, setEntityName] = React.useState<string>("");
    const [etype, setEntityType] = React.useState<string>("");

    const nameParam = React.useMemo(() => {
        const {type, id, name, description, unit, model} = entityName;
        const cloned = new ParameterType(type, id, name, description, unit, model);
        cloned.refPath = entityName.refPath;
        cloned.advanced = entityName.advanced;
        cloned.readOnly = false;
        cloned.value = "";
        return cloned;
    }, [entityName]);

    const changeEntityName = React.useCallback(() => {
        setEntityName(nameParam.value);
    }, [nameParam]);

    const typeParam = React.useMemo(() => {
        const {type, id, name, description, unit, model} = entityType;
        const cloned = new ParameterType(type, id, name, description, unit, model);
        cloned.refPath = entityType.refPath;
        cloned.advanced = entityType.advanced;
        cloned.readOnly = false;
        cloned.value = "";
        return cloned;
    }, [entityType]);

    const changeEntityType = React.useCallback(() => {
        setEntityType(typeParam.value);
    }, [typeParam]);

    const reset = React.useCallback(() => {
        nameParam.value = "";
        typeParam.value = "";
        setEntityName("");
        setEntityType("");
    }, [nameParam, typeParam]);

    const handleClose = React.useCallback(() => {
        onClose();
        reset();
    }, [reset, onClose]);

    const handleValidate = React.useCallback((event: React.FormEvent<HTMLFormElement>) => {
        event.preventDefault();
        onValidate(entity, etype);
        reset();
    }, [onValidate, reset, entity, etype]);

    return (
        <Dialog open={open} onClose={handleClose}>
            <form onSubmit={handleValidate}>
                <DialogTitle>Add a new Entity in your Platform</DialogTitle>
                <DialogContent>
                    <DialogContentText>
                        Please give a name and select the role of your machine.
                    </DialogContentText>
                    <Parameter parameter={nameParam} changeModel={changeEntityName} actions={noActions} />
                    <Parameter parameter={typeParam} changeModel={changeEntityType} actions={noActions} />
                </DialogContent>
                <DialogActions>
                    <Button onClick={handleClose} color="primary">Cancel</Button>
                    <Button type="submit" color="primary">Add Entity</Button>
                </DialogActions>
            </form>
        </Dialog>
    );
};


export default NewEntityDialog;
