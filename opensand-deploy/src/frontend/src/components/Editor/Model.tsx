import React from 'react';

import Button from '@material-ui/core/Button';
import Box from '@material-ui/core/Box';
import FormControl from '@material-ui/core/FormControl';
import InputLabel from '@material-ui/core/InputLabel';
import MenuItem from '@material-ui/core/MenuItem';
import Paper from '@material-ui/core/Paper';
import Select from '@material-ui/core/Select';
import Toolbar from '@material-ui/core/Toolbar';
import Typography from '@material-ui/core/Typography';

import {makeStyles, Theme} from '@material-ui/core/styles';

import {updateProjectXML, silenceSuccess, IApiSuccess} from '../../api';
import {sendError} from '../../utils/dispatcher';
import {Model as ModelType, Visibility, Visibilities} from '../../xsd/model';

import RootComponent from '../Model/RootComponent';
import SaveAsButton from './SaveAsButton';


interface Props {
    model: ModelType;
    projectName: string;
    urlFragment: string;
    xsd: string | null;
    reloadModel: () => void;
}


const useStyles = makeStyles((theme: Theme) => ({
    description: {
        marginRight: theme.spacing(2),
        textTransform: "capitalize",
        fontVariant: "small-caps",
    },
    version: {
        flexGrow: 1,
    },
    fullHeight: {
        height: "100%",
    },
    button: {
        marginRight: theme.spacing(2),
    },
}));


const Model = (props: Props) => {
    const {model, xsd, projectName, urlFragment, reloadModel} = props;
    const {root, version} = model;

    const [, setState] = React.useState<object>({});
    const [visibility, setVisibility] = React.useState<Visibility>("NORMAL");
    const classes = useStyles();

    const validateSaved = React.useCallback((status: IApiSuccess) => {
        model.saved = true;
        setState({});
        reloadModel();
    }, [model, setState, reloadModel]);

    const handleSave = React.useCallback(() => {
        updateProjectXML(validateSaved, sendError, projectName, urlFragment, model);
    }, [validateSaved, projectName, urlFragment, model]);

    const handleSaveAs = React.useCallback((template: string) => {
        const url = "template/" + xsd + "/" + template;
        updateProjectXML(silenceSuccess, sendError, projectName, url, model);
    }, [projectName, xsd, model]);

    const changeVisibility = React.useCallback((event: React.ChangeEvent<{name?: string; value: unknown;}>) => {
        const visibility = event.target.value as Visibility;
        model.visibility = visibility;
        setVisibility(visibility);
    }, [model, setVisibility]);

    const changeModel = React.useCallback(() => {
        model.saved = false;
        setState({});
    }, [model, setState]);

    return (
        <Paper elevation={0} className={classes.fullHeight}>
            <Toolbar>
                <Typography variant="h6" className={classes.description}>
                    {root.description}
                </Typography>
                <Typography variant="h6" className={classes.version}>
                    (v{version})
                </Typography>
                <FormControl>
                    <InputLabel htmlFor="visibility">Visibility</InputLabel>
                    <Select value={visibility} onChange={changeVisibility} inputProps={{id: "visibility"}}>
                        {Visibilities.map((v: Visibility, i: number) => <MenuItem value={v} key={i}>{v}</MenuItem>)}
                    </Select>
                </FormControl>
            </Toolbar>
            <RootComponent root={root} modelChanged={changeModel} />
            <Box textAlign="center" marginTop="3em" marginBottom="3px">
                <SaveAsButton disabled={xsd == null} onSave={handleSaveAs} />
                <Button
                    className={classes.button}
                    disabled={model.saved}
                    color="secondary"
                    variant="contained"
                    onClick={handleSave}
                >
                    Save Configuration for {urlFragment.replace("/", " of ")}
                </Button>
            </Box>
        </Paper>
    );
};


export default Model;
