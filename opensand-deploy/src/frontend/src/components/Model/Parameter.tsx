import React from 'react';
import {useParams} from 'react-router-dom';
import {useField} from 'formik';

import Checkbox from '@mui/material/Checkbox';
import CircularProgress from '@mui/material/CircularProgress';
import FormControlLabel from '@mui/material/FormControlLabel';
import IconButton from '@mui/material/IconButton';
import InputAdornment from '@mui/material/InputAdornment';
import MenuItem from '@mui/material/MenuItem';
import Stack from '@mui/material/Stack';
import TextField from '@mui/material/TextField';
import Tooltip from '@mui/material/Tooltip';

import DeleteIcon from '@mui/icons-material/HighlightOff';
import EditIcon from '@mui/icons-material/Edit';
import HelpIcon from '@mui/icons-material/Help';

import {forceEntityInXML} from '../../api';
import {useSelector, useDispatch} from '../../redux';
import type {IXsdAction} from '../../utils/actions';
import {getXsdName} from '../../xsd';
import type {Parameter as ParameterType} from '../../xsd';


const BooleanParam: React.FC<BaseProps> = (props) => {
    const {parameter, readOnly, name} = props;
    const [{value, onChange, onBlur},, {setValue}] = useField<string>(name);

    const handleChange = React.useCallback((event: React.ChangeEvent<HTMLInputElement>) => {
        onChange(event);
        setValue(String(event.target.checked));
    }, [onChange, setValue]);

    const help = parameter.description === "" ? null : (
        <Tooltip title={parameter.description} placement="top">
            <IconButton><HelpIcon /></IconButton>
        </Tooltip>
    );

    const checkbox = (
        <Checkbox
            name={name}
            checked={value === "true"}
            onChange={handleChange}
            onBlur={onBlur}
            disabled={readOnly}
        />
    );

    return (
        <Stack direction="row" mb={1}>
            <FormControlLabel control={checkbox} label={parameter.name} />
            {help}
        </Stack>
    );
};


const NumberParam: React.FC<NumberProps> = (props) => {
    const {parameter, readOnly, min, max, step, name} = props;
    const [{value, onChange, onBlur}] = useField<string>(name);

    return (
        <Stack direction="row" mb={1}>
            <TextField
                variant="outlined"
                name={name}
                label={parameter.name}
                value={value}
                onChange={onChange}
                onBlur={onBlur}
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
        </Stack>
    );
};


const StringParam: React.FC<StringProps> = (props) => {
    const {parameter, readOnly, lengthLimit, name} = props;
    const [{value, onChange, onBlur}] = useField<string>(name);

    const inputProps = lengthLimit ? {pattern: "?".repeat(lengthLimit)} : undefined;
    return (
        <Stack direction="row" mb={1}>
            <TextField
                variant="outlined"
                name={name}
                label={parameter.name}
                value={value}
                onChange={onChange}
                onBlur={onBlur}
                fullWidth
                autoFocus
                disabled={readOnly}
                inputProps={inputProps}
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
        </Stack>
    );
};


const EnumParam: React.FC<EnumProps> = (props) => {
    const {parameter, readOnly, enumeration, name} = props;
    const [{value, onChange, onBlur}] = useField<string>(name);

    const header = React.useMemo(() => <em>Please select a {parameter.name}</em>, [parameter.name]);

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

    return (
        <Stack direction="row" mb={1}>
            <TextField
                select
                fullWidth
                name={name}
                label={value ? parameter.name : null}
                value={value}
                onChange={onChange}
                onBlur={onBlur}
                disabled={readOnly}
                SelectProps={{
                    displayEmpty: true,
                    renderValue
                }}
            >
                {choices}
            </TextField>
            {help}
        </Stack>
    );
};


export const XsdParameter: React.FC<XsdProps> = (props) => {
    const {parameter, readOnly, entity, actions: {onEdit, onRemove}, name} = props;
    const [{value, onChange, onBlur},, {setValue}] = useField<string>(name);

    const loading = useSelector((state) => state.model.status);
    const templates = useSelector((state) => state.form.templates);
    const dispatch = useDispatch();
    const url = useParams();

    const [onSubmitted, setSubmitted] = React.useState<(() => void) | undefined>(undefined);
    const [saved, setSaved] = React.useState<boolean>(false);

    const parameter_key = React.useMemo(() => parameter.id.substr(0, parameter.id.indexOf("__template")), [parameter.id]);
    const xsd = React.useMemo(() => getXsdName(parameter_key, entity?.type), [parameter_key, entity]);

    const handleChange = React.useCallback((event: React.ChangeEvent<HTMLInputElement>) => {
        onChange(event);
        const value = event.target.value || "";

        if (value !== "") {
            if (onEdit) {
                if (xsd === "infrastructure.xsd") {
                    setSubmitted(() => () => {
                        if (url.name && entity) {
                            const saveUrl = parameter_key + "/" + entity.name;
                            const loadUrl = `template/${xsd}/${value}`;
                            dispatch(forceEntityInXML({project: url.name, xsd, loadUrl, saveUrl, entity: entity.type}));
                            setSaved(true);
                            setSubmitted(() => () => {
                                setSubmitted(undefined);
                                onEdit(entity, parameter_key, xsd);
                            });
                        } else {
                            setSubmitted(undefined);
                            onEdit(entity, parameter_key, xsd, value);
                        }
                    });
                } else {
                    setSubmitted(() => () => onEdit(entity, parameter_key, xsd, value));
                }
            }
        } else {
            onRemove && onRemove(entity?.name, parameter_key);
        }
    }, [dispatch, url.name, parameter_key, xsd, entity, onEdit, onRemove, onChange]);

    const handleClear = React.useCallback(() => {
        setValue("");
        onRemove && onRemove(entity?.name, parameter_key);
    }, [setValue, parameter_key, entity, onRemove]);

    const handleEdit = React.useCallback(() => {
        onEdit && onEdit(entity, parameter_key, xsd);
    }, [parameter_key, xsd, entity, onEdit]);

    React.useEffect(() => {
        if (onSubmitted && loading === "saved") {
            setSaved(true);
        }
        if (onSubmitted && saved && loading === "success") {
            setSaved(false);
            onSubmitted();
        }
    }, [loading, onSubmitted, saved]);

    const templatesNames = React.useMemo(() => templates[xsd] || [], [templates, xsd]);
    const header = React.useMemo(() => (
        <em>
            {templatesNames.length ? "Please select a template for the" : "This entity does not require a"} {parameter.name}
        </em>
    ), [templatesNames.length, parameter.name]);
    const hasValue = React.useMemo(() => value && value !== "", [value]);

    const renderValue = React.useCallback((selected: any) => {
        if (selected == null || selected === "") {
            return header;
        } else {
            return selected;
        }
    }, [header]);

    const choices = templatesNames.map((v: string, i: number) => <MenuItem value={v} key={i+1}>{v}</MenuItem>);
    choices.splice(0, 0, <MenuItem value="" key={0}>{header}</MenuItem>);

    return (
        <Stack direction="row" mb={1}>
            <TextField
                select
                fullWidth
                name={name}
                label={hasValue ? parameter.name : null}
                value={templatesNames.length ? value : ""}
                onChange={handleChange}
                onBlur={onBlur}
                disabled={readOnly || !templatesNames.length}
                SelectProps={{
                    displayEmpty: true,
                    renderValue
                }}
                InputProps={{
                    endAdornment: <InputAdornment position="end">
                        {hasValue && (
                            <Tooltip placement="top" title="Edit this configuration file">
                                <IconButton onClick={handleEdit} disabled={readOnly}>
                                    <EditIcon />
                                </IconButton>
                            </Tooltip>
                        )}
                        {hasValue && (
                            <Tooltip placement="top" title="Remove this configuration file">
                                <IconButton onClick={handleClear} disabled={readOnly} sx={{mr: 2}}>
                                    <DeleteIcon />
                                </IconButton>
                            </Tooltip>
                        )}
                        {onSubmitted != null && loading === "pending" && (
                            <CircularProgress />
                        )}
                    </InputAdornment>,
                }}
            >
                {choices}
            </TextField>
        </Stack>
    );
};


const Parameter: React.FC<Props> = (props) => {
    const {parameter, readOnly, prefix, ...rest} = props;

    const model = useSelector((state) => state.model.model);

    const isReadOnly = readOnly || parameter.readOnly;
    const name = prefix + ".value";

    const enumeration = model?.environment?.enums?.find(e => e.id === parameter.type);
    if (enumeration != null) {
        return (
            <EnumParam
                parameter={parameter}
                readOnly={isReadOnly}
                enumeration={enumeration.values}
                name={name}
                {...rest}
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
                    name={name}
                    {...rest}
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
                    name={name}
                    {...rest}
                />
            );
        case "byte":
            return (
                <NumberParam
                    parameter={parameter}
                    readOnly={isReadOnly}
                    min={-128}
                    max={127}
                    step={1}
                    name={name}
                    {...rest}
                />
            );
        case "ubyte":
            return (
                <NumberParam
                    parameter={parameter}
                    readOnly={isReadOnly}
                    min={0}
                    max={255}
                    step={1}
                    name={name}
                    {...rest}
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
                    name={name}
                    {...rest}
                />
            );
        case "ushort":
            return (
                <NumberParam
                    parameter={parameter}
                    readOnly={isReadOnly}
                    min={0}
                    max={65535}
                    step={1}
                    name={name}
                    {...rest}
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
                    name={name}
                    {...rest}
                />
            );
        case "uint":
        case "size":
            return (
                <NumberParam
                    parameter={parameter}
                    readOnly={isReadOnly}
                    min={0}
                    max={4294967295}
                    step={1}
                    name={name}
                    {...rest}
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
                    name={name}
                    {...rest}
                />
            );
        case "ulong":
            return (
                <NumberParam
                    parameter={parameter}
                    readOnly={isReadOnly}
                    min={0}
                    max={Number.MAX_SAFE_INTEGER}
                    step={1}
                    name={name}
                    {...rest}
                />
            );
        case "char":
            return (
                <StringParam
                    parameter={parameter}
                    readOnly={isReadOnly}
                    lengthLimit={1}
                    name={name}
                    {...rest}
                />
            );
        case "string":
        default:
            return (
                <StringParam
                    parameter={parameter}
                    readOnly={isReadOnly}
                    name={name}
                    {...rest}
                />
            );
    }
};


interface BaseProps {
    parameter: ParameterType;
    readOnly?: boolean;
    name: string;
}


interface Props extends Omit<BaseProps, "name"> {
    prefix: string;
}


interface StringProps extends BaseProps {
    lengthLimit?: number;
}


interface NumberProps extends BaseProps {
    min: number;
    max: number;
    step: number;
}


interface EnumProps extends BaseProps {
    enumeration: string[];
}


interface XsdProps extends BaseProps {
    entity?: {name: string; type: string;};
    actions: IXsdAction;
}


export default Parameter;
