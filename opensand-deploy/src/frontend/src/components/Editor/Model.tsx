import React from 'react';
import {Formik} from 'formik';
import type {FormikProps, FormikHelpers} from 'formik';

import Box from '@mui/material/Box';
import MenuItem from '@mui/material/MenuItem';
import Paper from '@mui/material/Paper';
import TextField from '@mui/material/TextField';
import Toolbar from '@mui/material/Toolbar';
import Typography from '@mui/material/Typography';

import {styled} from '@mui/material/styles';

import GrowingTypography from '../common/GrowingTypography';
import RootComponent from '../Model/RootComponent';
import SaveAsButton, {SpacedButton} from './SaveAsButton';

import {updateXML} from '../../api';
import {useDispatch, useSelector} from '../../redux';
import {changeVisibility} from '../../redux/form';
import {useFormSubmit} from '../../utils/hooks';
import {Visibilities} from '../../xsd';
import type {Model as ModelType, Component, Visibility} from '../../xsd';


const CapitalizedTypography = styled(Typography, {name: "CapitalizedTypography", slot: "Wrapper"})(({ theme }) => ({
    marginRight: theme.spacing(2),
    textTransform: "capitalize",
    fontVariant: "small-caps",
}));


const FullPage = styled(Paper, {name: "FullPage", slot: "Wrapper"})({
    height: "100%",
});


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
        <FullPage elevation={0}>
            <Toolbar>
                <CapitalizedTypography variant="h6">
                    {root.description}
                </CapitalizedTypography>
                <GrowingTypography variant="h6">
                    (v{version})
                </GrowingTypography>
                <TextField select label="Visibility" value={visibility} onChange={handleVisibilityChange}>
                    {Visibilities.map((v: Visibility, i: number) => <MenuItem value={v} key={i}>{v}</MenuItem>)}
                </TextField>
            </Toolbar>
            <Formik enableReinitialize initialValues={root} onSubmit={handleSubmit}>
                {(formik: FormikProps<Component>) => (<form onSubmit={formik.handleSubmit}>
                    <RootComponent form={formik} xsd={xsd} />
                    <Box textAlign="center" marginTop="3em" marginBottom="3px">
                        <SaveAsButton project={projectName} xsd={xsd} form={formik} />
                        <SpacedButton
                            disabled={!formik.dirty || formik.isSubmitting}
                            color="secondary"
                            variant="contained"
                            type="submit"
                        >
                            Save Configuration for {urlFragment.replace("/", " of ")}
                        </SpacedButton>
                    </Box>
                </form>)}
            </Formik>
        </FullPage>
    );
};


interface Props {
    model: ModelType;
    projectName: string;
    urlFragment: string;
    xsd: string;
}


export default Model;
