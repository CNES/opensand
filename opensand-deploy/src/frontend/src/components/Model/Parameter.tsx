import React from 'react';

import Checkbox from '@material-ui/core/Checkbox';
import FormControl from '@material-ui/core/FormControl';
import FormControlLabel from '@material-ui/core/FormControlLabel';
import IconButton from '@material-ui/core/IconButton';
import InputAdornment from '@material-ui/core/InputAdornment';
import InputLabel from '@material-ui/core/InputLabel';
import MenuItem from '@material-ui/core/MenuItem';
import Select from '@material-ui/core/Select';
import TextField from '@material-ui/core/TextField';
import Tooltip from '@material-ui/core/Tooltip';

import HelpIcon from '@material-ui/icons/Help';

import {parameterStyles} from '../../utils/theme';
import {Parameter as ParameterType} from '../../xsd/model';


interface Props {
    parameter: ParameterType;
    changeModel: () => void;
}


interface NumberProps extends Props {
    min: number;
    max: number;
    step: number;
}


interface EnumProps extends Props {
    enumeration: string[];
}


const BooleanParam = (props: Props) => {
    const {parameter, changeModel} = props;
    const classes = parameterStyles();

    const handleChange = React.useCallback((event: React.ChangeEvent<HTMLInputElement>) => {
        parameter.value = String(parameter.value !== "true");
        changeModel();
    }, [parameter, changeModel]);

    const help = parameter.description === "" ? null : (
        <Tooltip title={parameter.description} placement="top">
            <IconButton><HelpIcon /></IconButton>
        </Tooltip>
    );

    // Need to wrap in a <div> not a <React.Fragment> for the display block
    return (
        <div className={classes.spaced}>
            <FormControlLabel
                control={<Checkbox checked={parameter.value === "true"} onChange={handleChange} />}
                label={parameter.name}
            />
            {help}
        </div>
    );
};


const NumberParam = (props: NumberProps) => {
    const {parameter, min, max, step, changeModel} = props;
    const classes = parameterStyles();

    const handleChange = React.useCallback((event: React.ChangeEvent<HTMLInputElement>) => {
        parameter.value = event.target.value;
        changeModel();
    }, [parameter, changeModel]);

    const handleBlur = React.useCallback(() => {
        const saved = parameter.model.saved;
        changeModel();
        parameter.model.saved = saved;
    }, [parameter, changeModel]);

    // label={ parameter.name + " (" + parameter.type + ")" } ???
    // Need to wrap in a <div> not a <React.Fragment> for the display block
    return (
        <div className={classes.spaced}>
            <TextField
                variant="outlined"
                label={parameter.name}
                value={parameter.value}
                onChange={handleChange}
                onBlur={handleBlur}
                fullWidth
                type="number"
                InputProps={{
                    endAdornment: <InputAdornment position="end">
                        { parameter.unit }
                        { parameter.description !== "" &&
                            <Tooltip title={parameter.description} placement="top">
                                <IconButton><HelpIcon /></IconButton>
                            </Tooltip>
                        }
                    </InputAdornment>,
                    inputProps: {min, max, step},
                }}
            />
        </div>
    );
};


const StringParam = (props: Props) => {
    const {parameter, changeModel} = props;
    const classes = parameterStyles();

    const handleChange = React.useCallback((event: React.ChangeEvent<HTMLInputElement>) => {
        parameter.value = event.target.value;
        changeModel();
    }, [parameter, changeModel]);

    const handleBlur = React.useCallback(() => {
        const saved = parameter.model.saved;
        changeModel();
        parameter.model.saved = saved;
    }, [parameter, changeModel]);

    // label={ parameter.name + " (" + parameter.type + ")" } ???
    // Need to wrap in a <div> not a <React.Fragment> for the display block
    return (
        <div className={classes.spaced}>
            <TextField
                variant="outlined"
                label={parameter.name}
                value={parameter.value}
                onChange={handleChange}
                onBlur={handleBlur}
                fullWidth
                InputProps={{
                    endAdornment: <InputAdornment position="end">
                        { parameter.unit }
                        { parameter.description !== "" &&
                            <Tooltip title={parameter.description} placement="top">
                                <IconButton><HelpIcon /></IconButton>
                            </Tooltip>
                        }
                    </InputAdornment>,
                }}
            />
        </div>
    );
};


const EnumParam = (props: EnumProps) => {
    const {parameter, changeModel, enumeration} = props;
    const classes = parameterStyles();

    const handleChange = React.useCallback((event: React.ChangeEvent<{name?: string; value: unknown;}>) => {
        parameter.value = event.target.value as string;
        changeModel();
    }, [parameter, changeModel]);

    const header = React.useMemo(() => <em>Please select a {parameter.name}</em>, [parameter]);

    const renderValue = React.useCallback((selected: any) => {
        if (selected == null || selected === "") {
            return header;
        } else {
            return selected;
        }
    }, [header]);

    const help = parameter.description === "" ? null : (
        <Tooltip title={parameter.description} placement="top">
            <IconButton><HelpIcon /></IconButton>
        </Tooltip>
    );
    const choices = enumeration.map((v: string, i: number) => <MenuItem value={v} key={i+1}>{v}</MenuItem>);
    choices.splice(0, 0, <MenuItem value="" key={0}>{header}</MenuItem>);

    // Need to wrap in a <div> not a <React.Fragment> for the display block
    return (
        <div className={classes.spaced}>
            <FormControl className={classes.fullWidth}>
                <InputLabel htmlFor={parameter.id}>
                    {parameter.value === "" ? null : parameter.name}
                </InputLabel>
                <Select
                    value={parameter.value}
                    onChange={handleChange}
                    inputProps={{id: parameter.id}}
                    displayEmpty
                    renderValue={renderValue}
                >
                    {choices}
                </Select>
            </FormControl>
            {help}
        </div>
    );
};


const Parameter = (props: Props) => {
    const {parameter, changeModel} = props;

    const enumeration = parameter.model.environment.enums.find(e => e.id === parameter.type);
    if (enumeration != null) {
        return <EnumParam parameter={parameter} enumeration={enumeration.values} changeModel={changeModel} />;
    }

    switch (parameter.type) {
        case "bool":
        case "boolean":
            return <BooleanParam parameter={parameter} changeModel={changeModel} />;
        case "longdouble":
        case "double":
        case "float":
            return (
                <NumberParam
                    parameter={parameter}
                    min={Number.MIN_SAFE_INTEGER}
                    max={Number.MAX_SAFE_INTEGER}
                    step={0.01}
                    changeModel={changeModel}
                />
            );
        case "byte":
            return (
                <NumberParam
                    parameter={parameter}
                    min={0}
                    max={255}
                    step={1}
                    changeModel={changeModel}
                />
            );
        case "short":
            return (
                <NumberParam
                    parameter={parameter}
                    min={-32768}
                    max={32767}
                    step={1}
                    changeModel={changeModel}
                />
            );
        case "int":
            return (
                <NumberParam
                    parameter={parameter}
                    min={-2147483648}
                    max={2147483647}
                    step={1}
                    changeModel={changeModel}
                />
            );
        case "long":
            return (
                <NumberParam
                    parameter={parameter}
                    min={Number.MIN_SAFE_INTEGER}
                    max={Number.MAX_SAFE_INTEGER}
                    step={1}
                    changeModel={changeModel}
                />
            );
        case "string":
        case "char":
        default:
            return <StringParam parameter={parameter} changeModel={changeModel} />;
    }
};


export default Parameter;
