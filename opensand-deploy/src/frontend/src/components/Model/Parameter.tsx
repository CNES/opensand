import React from 'react';

import Checkbox from '@material-ui/core/Checkbox';
import FormControl from '@material-ui/core/FormControl';
import FormControlLabel from '@material-ui/core/FormControlLabel';
import FormHelperText from '@material-ui/core/FormHelperText';
import IconButton from '@material-ui/core/IconButton';
import InputAdornment from '@material-ui/core/InputAdornment';
import InputLabel from '@material-ui/core/InputLabel';
import MenuItem from '@material-ui/core/MenuItem';
import Select from '@material-ui/core/Select';
import TextField from '@material-ui/core/TextField';
import Tooltip from '@material-ui/core/Tooltip';

import DeleteIcon from '@material-ui/icons/HighlightOff';
import EditIcon from '@material-ui/icons/Edit';
import HelpIcon from '@material-ui/icons/Help';

import {
    listProjectTemplates,
    IActions,
    ITemplatesContent,
} from '../../api';
import {sendError} from '../../utils/dispatcher';
import {useDidMount} from '../../utils/hooks';
import {parameterStyles} from '../../utils/theme';
import {Parameter as ParameterType, isComponentElement, isParameterElement} from '../../xsd/model';


interface Props {
    parameter: ParameterType;
    readOnly?: boolean;
    entity?: string;
    changeModel: () => void;
    actions?: IActions;
}


const BooleanParam = (props: Props) => {
    const {parameter, readOnly, changeModel} = props;
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

    const checkbox = (
        <Checkbox
            checked={parameter.value === "true"}
            onChange={handleChange}
            disabled={readOnly}
        />
    );

    // Need to wrap in a <div> not a <React.Fragment> for the display block
    return (
        <div className={classes.spaced}>
            <FormControlLabel control={checkbox} label={parameter.name} />
            {help}
        </div>
    );
};


interface NumberProps extends Props {
    min: number;
    max: number;
    step: number;
}


const NumberParam = (props: NumberProps) => {
    const {parameter, readOnly, min, max, step, changeModel} = props;
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
                disabled={readOnly}
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
    const {parameter, readOnly, changeModel} = props;
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
                disabled={readOnly}
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


interface EnumProps extends Props {
    enumeration: string[];
}


const EnumParam = (props: EnumProps) => {
    const {parameter, readOnly, changeModel, enumeration} = props;
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
                    disabled={readOnly}
                    renderValue={renderValue}
                >
                    {choices}
                </Select>
            </FormControl>
            {help}
        </div>
    );
};


interface XsdEnumProps extends Props {
}


// TODO:
// add onSelect action
// add onEdit action
// add onDelete action
const XsdParameter = (props: XsdEnumProps) => {
    const {parameter, readOnly, changeModel, entity, actions} = props;
    const {onEdit, onSelect, onDelete} = actions?.$ || {};

    const didMount = useDidMount();
    const classes = parameterStyles();

    const [templates, setTemplates] = React.useState<ITemplatesContent>({});

    // TODO improve for profile
    const xsd = React.useMemo(() => parameter.id === "profile" ? `profile_${entity}.xsd` : `${parameter.id}.xsd`, [parameter.id, entity]);

    const handleChange = React.useCallback((event: React.ChangeEvent<{name?: string; value: unknown;}>) => {
        const value = event.target.value as string;
        if (value == null) {
            return;
        }

        parameter.value = xsd;
        onEdit && onEdit(parameter.id, xsd, value);
    }, [parameter, xsd, onEdit]);

    const handleClear = React.useCallback(() => {
        parameter.value = "";
        onDelete && onDelete(parameter.id);
    }, [parameter, onDelete]);

    const handleEdit = React.useCallback(() => {
        onEdit && onEdit(parameter.id, parameter.value);
    }, [parameter, onEdit]);

    const header = React.useMemo(() => <em>Please select a template</em>, []);
    const hasValue = React.useMemo(() => parameter.value && parameter.value !== "", [parameter.value]);

    const projectName = React.useMemo(() => {
        const platform = parameter.model.root.elements.find(e => e.element.id === "platform");
        if (isComponentElement(platform)) {
            const project = platform.element.elements.find(e => e.element.id === "project");
            if (isParameterElement(project)) {
                return project.element.value;
            }
        }

        return "";
    }, [parameter.model]);

    const renderValue = React.useCallback((selected: any) => {
        if (selected == null || selected === "") {
            return header;
        } else {
            return selected;
        }
    }, [header]);

    React.useEffect(() => {
        if (hasValue && didMount && onSelect) onSelect();
        listProjectTemplates(setTemplates, sendError, projectName);
        return () => {setTemplates({});};
    }, [onSelect, didMount, hasValue, projectName]);

    const templatesNames = templates[xsd] || [];
    const choices = templatesNames.map((v: string, i: number) => <MenuItem value={v} key={i+1}>{v}</MenuItem>);
    choices.splice(0, 0, <MenuItem value="" key={0}>{header}</MenuItem>);

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
                    disabled={entity === "sat"}
                >
                    {choices}
                </Select>
            </FormControl>
            {hasValue && (
                <Tooltip placement="top" title="Edit this configuration file">
                    <IconButton onClick={handleEdit}>
                        <EditIcon />
                    </IconButton>
                </Tooltip>
            )}
            {hasValue && (
                <Tooltip placement="top" title="Remove this configuration file">
                    <IconButton onClick={handleClear}>
                        <DeleteIcon />
                    </IconButton>
                </Tooltip>
            )}
        </div>
    );
};


const Parameter = (props: Props) => {
    const {parameter, readOnly, entity, changeModel, actions} = props;

    const isReadOnly = readOnly || parameter.readOnly;

    if (parameter.type.endsWith("_xsd")) {
        return (
            <XsdParameter
                parameter={parameter}
                readOnly={isReadOnly}
                changeModel={changeModel}
                actions={actions}
            />
        );
    }

    const enumeration = parameter.model.environment.enums.find(e => e.id === parameter.type);
    if (enumeration != null) {
        return (
            <EnumParam
                parameter={parameter}
                readOnly={isReadOnly}
                enumeration={enumeration.values}
                changeModel={changeModel}
                actions={actions}
            />
        );
    }

    switch (parameter.type) {
        case "bool":
        case "boolean":
            return (
                <BooleanParam
                    parameter={parameter}
                    readOnly={isReadOnly}
                    changeModel={changeModel}
                    actions={actions}
                />
            );
        case "longdouble":
        case "double":
        case "float":
            return (
                <NumberParam
                    parameter={parameter}
                    readOnly={isReadOnly}
                    min={Number.MIN_SAFE_INTEGER}
                    max={Number.MAX_SAFE_INTEGER}
                    step={0.01}
                    changeModel={changeModel}
                    actions={actions}
                />
            );
        case "byte":
            return (
                <NumberParam
                    parameter={parameter}
                    readOnly={isReadOnly}
                    min={0}
                    max={255}
                    step={1}
                    changeModel={changeModel}
                    actions={actions}
                />
            );
        case "short":
            return (
                <NumberParam
                    parameter={parameter}
                    readOnly={isReadOnly}
                    min={-32768}
                    max={32767}
                    step={1}
                    changeModel={changeModel}
                    actions={actions}
                />
            );
        case "int":
            return (
                <NumberParam
                    parameter={parameter}
                    readOnly={isReadOnly}
                    min={-2147483648}
                    max={2147483647}
                    step={1}
                    changeModel={changeModel}
                    actions={actions}
                />
            );
        case "long":
            return (
                <NumberParam
                    parameter={parameter}
                    readOnly={isReadOnly}
                    min={Number.MIN_SAFE_INTEGER}
                    max={Number.MAX_SAFE_INTEGER}
                    step={1}
                    changeModel={changeModel}
                    actions={actions}
                />
            );
        case "string":
        case "char":
        default:
            return (
                <StringParam
                    parameter={parameter}
                    readOnly={isReadOnly}
                    changeModel={changeModel}
                    actions={actions}
                />
            );
    }
};


export default Parameter;
