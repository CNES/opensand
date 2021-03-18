import React from 'react';

import AppBar from '@material-ui/core/AppBar';
import Button from '@material-ui/core/Button';
import Box from '@material-ui/core/Box';
import FormControl from '@material-ui/core/FormControl';
import InputLabel from '@material-ui/core/InputLabel';
import MenuItem from '@material-ui/core/MenuItem';
import Paper from '@material-ui/core/Paper';
import Select from '@material-ui/core/Select';
import Tab from '@material-ui/core/Tab';
import Tabs from '@material-ui/core/Tabs';
import Toolbar from '@material-ui/core/Toolbar';
import Tooltip from '@material-ui/core/Tooltip';
import Typography from '@material-ui/core/Typography';

import {makeStyles, Theme} from '@material-ui/core/styles';

import {updateProjectXML, silenceSuccess, IApiSuccess} from '../../api';
import {sendError} from '../../utils/dispatcher';
import {Model as ModelType, Component as ComponentType, Visibility, Visibilities} from '../../xsd/model';

import Component from './Component';
import SaveAsButton from './SaveAsButton';
import SingleListComponent from './SingleListComponent';


interface Props {
    model: ModelType;
    projectName: string;
    urlFragment: string;
    xsd: string | null;
}


interface PanelProps {
    className?: string;
    children?: React.ReactNode;
    index: any;
    value: any;
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


const TabPanel = (props: PanelProps) => {
    const {children, value, index, ...other} = props;

    return (
        <div hidden={value !== index} {...other}>
            {value === index && children}
        </div>
    );
};


const Model = (props: Props) => {
    const {model, xsd, projectName, urlFragment} = props;
    const {root, version} = model;
    const components = root.getComponents();

    const [value, setValue] = React.useState<number>(0);
    const [, setState] = React.useState<object>({});
    const [visibility, setVisibility] = React.useState<Visibility>("NORMAL");
    const classes = useStyles();

    const validateSaved = React.useCallback((status: IApiSuccess) => {
        model.saved = true;
        setState({});
    }, [model, setState]);

    const handleSave = React.useCallback(() => {
        updateProjectXML(validateSaved, sendError, projectName, urlFragment, model);
    }, [validateSaved, projectName, urlFragment, model]);

    const handleSaveAs = React.useCallback((template: string) => {
        const url = "template/" + xsd + "/" + template;
        updateProjectXML(silenceSuccess, sendError, projectName, url, model);
    }, [projectName, xsd, model]);

    const handleChange = React.useCallback((event: React.ChangeEvent<{}>, index: number) => {
        setValue(index);
    }, [setValue]);

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
            <AppBar position="static" color="primary">
                <Tabs
                    value={value}
                    onChange={handleChange}
                    indicatorColor="secondary"
                    textColor="inherit"
                    variant="fullWidth"
                >
                    {components.map((c: ComponentType, i: number) => c.description === "" ? (
                        <Tab key={c.id} label={c.name} value={i} />
                    ) : (
                        <Tooltip title={c.description} placement="top" key={c.id}>
                            <Tab key={c.id} label={c.name} value={i} />
                        </Tooltip>
                    ))}
                </Tabs>
            </AppBar>
            {components.map((c: ComponentType, i: number) => (
                <TabPanel key={i} value={value} index={i}>
                    {c.elements.length === 1 && c.elements[0].type === "list" ? (
                        <SingleListComponent
                            list={c.elements[0].element}
                            readOnly={c.readOnly}
                            changeModel={changeModel}
                        />
                    ) : (
                        <Component component={c} changeModel={changeModel} />
                    )}
                </TabPanel>
            ))}
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
