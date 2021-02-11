import React from 'react';

import Button from '@material-ui/core/Button';

import {makeStyles, Theme} from '@material-ui/core/styles';

import SingleFieldDialog from '../common//SingleFieldDialog';


interface Props {
    disabled: boolean;
    onSave: (template: string) => void;
}


const useStyles = makeStyles((theme: Theme) => ({
    button: {
        marginRight: theme.spacing(2),
    },
}));


const SaveAsButton = (props: Props) => {
    const {disabled, onSave} = props;
    const [open, setOpen] = React.useState<boolean>(false);
    const classes = useStyles();

    const handleOpen = React.useCallback(() => {
        setOpen(true);
    }, [setOpen]);

    const handleClose = React.useCallback(() => {
        setOpen(false);
    }, [setOpen]);

    const handleSave = React.useCallback((template: string) => {
        setOpen(false);
        onSave(template);
    }, [setOpen, onSave]);

    return (
        <React.Fragment>
            <Button
                className={classes.button}
                disabled={disabled}
                color="secondary"
                variant="contained"
                onClick={handleOpen}
            >
                Save As Template
            </Button>
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


export default SaveAsButton;
