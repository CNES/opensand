import React from 'react';

import AppBar from '@material-ui/core/AppBar';
import Button from '@material-ui/core/Button';
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

import {Model as ModelType, Component as ComponentType, Visibility, Visibilities} from '../../xsd/model';
import {updateProjectXML, IApiSuccess} from '../../api';

import Component from './Component';
import SingleListComponent from './SingleListComponent';


interface Props {
    model: ModelType;
    projectName: string;
    urlFragment: string;
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
        minHeight: "100vh",
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
    const {model, projectName, urlFragment} = props;
    const {root, version} = model;
    const {description, children} = root;

    const [value, setValue] = React.useState<number>(0);
    const [, setState] = React.useState<object>({});
    const [visibility, setVisibility] = React.useState<Visibility>("NORMAL");
    const classes = useStyles();

    const validateSaved = React.useCallback((status: IApiSuccess) => {
        model.saved = true;
        setState({});
    }, [model, setState]);

    const handleSave = React.useCallback(() => {
        updateProjectXML(validateSaved, console.log, projectName, urlFragment, model);
    }, [validateSaved, projectName, urlFragment, model]);

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
                    {description}
                </Typography>
                <Typography variant="h6" className={classes.version}>
                    (v{version})
                </Typography>
                <Button
                    className={classes.button}
                    disabled={model.saved}
                    color="secondary"
                    variant="contained"
                    onClick={handleSave}
                >
                    Save
                </Button>
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
                    {children.map((c: ComponentType, i: number) => c.description === "" ? (
                        <Tab key={c.id} label={c.name} value={i} />
                    ) : (
                        <Tooltip title={c.description} placement="top" key={c.id}>
                            <Tab key={c.id} label={c.name} value={i} />
                        </Tooltip>
                    ))}
                </Tabs>
            </AppBar>
            {children.map((c: ComponentType, i: number) => (
                <TabPanel key={i} value={value} index={i}>
                    {c.lists.length === 1 && c.children.length === 0 && c.parameters.length === 0 ? (
                        <SingleListComponent list={c.lists[0]} changeModel={changeModel} />
                    ) : (
                        <Component component={c} changeModel={changeModel} />
                    )}
                </TabPanel>
            ))}
        </Paper>
    );
};


export default Model;
