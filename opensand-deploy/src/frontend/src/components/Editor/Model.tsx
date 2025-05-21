import React from 'react';
import {Formik, Form} from 'formik';
import type {FormikProps, FormikHelpers} from 'formik';

import Button from '@mui/material/Button';
import MenuItem from '@mui/material/MenuItem';
import Stack from '@mui/material/Stack';
import TextField from '@mui/material/TextField';
import Toolbar from '@mui/material/Toolbar';
import Typography from '@mui/material/Typography';

import RootComponent from '../Model/RootComponent';
import SaveAsButton from './SaveAsButton';

import {updateXML} from '../../api';
import {useDispatch, useSelector} from '../../redux';
import {changeVisibility} from '../../redux/form';
import {useFormSubmit} from '../../utils/hooks';
import {Visibilities} from '../../xsd';
import type {Model as ModelType, Component, Visibility} from '../../xsd';


const Model: React.FC<Props> = (props) => {
    const {model, xsd, projectName, urlFragment} = props;
    const {root, version} = model;

    const visibility = useSelector((state) => state.form.visibility);
    const dispatch = useDispatch();
    const onSubmitted = useFormSubmit();

    const handleVisibilityChange = React.useCallback((event: React.ChangeEvent<HTMLInputElement>) => {
        dispatch(changeVisibility(event.target.value as Visibility));
    }, [dispatch]);

    const handleSubmit = React.useCallback((values: Component, helpers: FormikHelpers<Component>) => {
        dispatch(updateXML({project: projectName, xsd, urlFragment, root: values}));
        onSubmitted(() => () => helpers.setSubmitting(false));
    }, [dispatch, projectName, xsd, urlFragment, onSubmitted]);

    return (
        <React.Fragment>
            <Toolbar>
                <Typography variant="h6" sx={{mr: 2, textTransform: "capitalize", fontVariant: "small-caps"}}>
                    {root.description}
                </Typography>
                <Typography variant="h6" sx={{flexGrow: 1}}>
                    (v{version})
                </Typography>
                <TextField select label="Visibility" value={visibility} onChange={handleVisibilityChange}>
                    {Visibilities.map((v: Visibility, i: number) => <MenuItem value={v} key={i}>{v}</MenuItem>)}
                </TextField>
            </Toolbar>
            <Formik enableReinitialize initialValues={root} onSubmit={handleSubmit}>
                {(formik: FormikProps<Component>) => (
                    <Form>
                        <RootComponent xsd={xsd} />
                        <Stack direction="row" justifyContent="center" alignItems="center" spacing={1} mt={1} mb={3}>
                            <SaveAsButton project={projectName} xsd={xsd} />
                            <Button
                                disabled={!formik.dirty || formik.isSubmitting}
                                color="secondary"
                                variant="contained"
                                type="submit"
                            >
                                Save Configuration for {urlFragment.replace("/", " of ")}
                            </Button>
                        </Stack>
                    </Form>
                )}
            </Formik>
        </React.Fragment>
    );
};


interface Props {
    model: ModelType;
    projectName: string;
    urlFragment: string;
    xsd: string;
}


export default Model;
