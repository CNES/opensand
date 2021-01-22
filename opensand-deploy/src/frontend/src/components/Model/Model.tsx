import React from 'react';

import AppBar from '@material-ui/core/AppBar';
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

import Component from './Component';


interface Props {
    model: ModelType;
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
    tab: {
        marginTop: theme.spacing(1),
    },
    fullHeight: {
        minHeight: "100vh",
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
    const classes = useStyles();
    const version = props.model.version;
    const {description, children} = props.model.root;

    const [value, setValue] = React.useState<number>(0);
    const [, setState] = React.useState<object>({});
    const [visibility, setVisibility] = React.useState<Visibility>("NORMAL");

    const handleChange = React.useCallback((event: React.ChangeEvent<{}>, index: number) => {
        setValue(index);
    }, [setValue]);

    const changeVisibility = React.useCallback((event: React.ChangeEvent<{name?: string; value: unknown;}>) => {
        const visibility = event.target.value as Visibility;
        props.model.visibility = visibility;
        setVisibility(visibility);
    }, [props.model, setVisibility]);

    const changeModel = React.useCallback(() => {
        props.model.saved = false;
        setState({});
    }, [props.model, setState]);

    return (
        <Paper elevation={0} className={classes.fullHeight}>
            <Toolbar>
                <Typography variant="h6" className={classes.description}>
                    {description}
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
                    {children.map((c: ComponentType, i: number) => c.description === "" ? (
                        <Tab key={c.id} label={c.name} value={i} />
                    ) : (
                        <Tooltip title={c.description} placement="top">
                            <Tab key={c.id} label={c.name} value={i} />
                        </Tooltip>
                    ))}
                </Tabs>
            </AppBar>
            {children.map((c: ComponentType, i: number) => (
                <TabPanel key={i} value={value} index={i} className={classes.tab}>
                    <Component component={c} changeModel={changeModel} />
                </TabPanel>
            ))}
        </Paper>
    );
};


export default Model;
