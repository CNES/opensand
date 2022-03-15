import React from 'react';
import {useParams} from 'react-router-dom';
import type {FormikProps} from 'formik';

import Checkbox from '@mui/material/Checkbox';
import CircularProgress from '@mui/material/CircularProgress';
import FormControlLabel from '@mui/material/FormControlLabel';
import IconButton from '@mui/material/IconButton';
import InputAdornment from '@mui/material/InputAdornment';
import MenuItem from '@mui/material/MenuItem';
import TextField from '@mui/material/TextField';
import Tooltip from '@mui/material/Tooltip';

import {styled} from '@mui/material/styles';
import DeleteIcon from '@mui/icons-material/HighlightOff';
import EditIcon from '@mui/icons-material/Edit';
import HelpIcon from '@mui/icons-material/Help';

import {forceEntityInXML} from '../../api';
import {useSelector, useDispatch} from '../../redux';
import type {IActions} from '../../utils/actions';
import {getXsdName} from '../../xsd';
import type {Parameter as ParameterType} from '../../xsd';


const FlexBox = styled('div')(({ theme }) => ({
    display: "flex",
    marginBottom: theme.spacing(1),
}));


const BooleanParam: React.FC<BaseProps> = (props) => {
    const {parameter, readOnly, name, form: {handleChange, setFieldValue, handleBlur}, autoSave} = props;

    const myHandleChange = React.useCallback((event: React.ChangeEvent<HTMLInputElement>) => {
        handleChange(event);
        setFieldValue(name, String(event.target.checked));
        autoSave();
    }, [handleChange, name, setFieldValue, autoSave]);

    const help = parameter.description === "" ? null : (
        <Tooltip title={parameter.description} placement="top">
            <IconButton><HelpIcon /></IconButton>
        </Tooltip>
    );

    const checkbox = (
        <Checkbox
            name={name}
            checked={parameter.value === "true"}
            onChange={myHandleChange}
            onBlur={handleBlur}
            disabled={readOnly}
        />
    );

    return (
        <FlexBox>
            <FormControlLabel control={checkbox} label={parameter.name} />
            {help}
        </FlexBox>
    );
};


const NumberParam: React.FC<NumberProps> = (props) => {
    const {parameter, readOnly, min, max, step, name, form: {handleChange, handleBlur}, autoSave} = props;

    const myHandleChange = React.useCallback((event: React.ChangeEvent<HTMLInputElement>) => {
        handleChange(event);
        autoSave();
    }, [handleChange, autoSave]);

    // label={ parameter.name + " (" + parameter.type + ")" } ???
    return (
        <FlexBox>
            <TextField
                variant="outlined"
                name={name}
                label={parameter.name}
                value={parameter.value}
                onChange={myHandleChange}
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
        </FlexBox>
    );
};


const StringParam: React.FC<BaseProps> = (props) => {
    const {parameter, readOnly, name, form: {handleChange, handleBlur}, autoSave} = props;

    const myHandleChange = React.useCallback((event: React.ChangeEvent<HTMLInputElement>) => {
        handleChange(event);
        autoSave();
    }, [handleChange, autoSave]);

    // label={ parameter.name + " (" + parameter.type + ")" } ???
    return (
        <FlexBox>
            <TextField
                variant="outlined"
                name={name}
                label={parameter.name}
                value={parameter.value}
                onChange={myHandleChange}
                onBlur={handleBlur}
                fullWidth
                autoFocus
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
        </FlexBox>
    );
};


const EnumParam: React.FC<EnumProps> = (props) => {
    const {parameter, readOnly, enumeration, name, form: {handleChange, handleBlur}, autoSave} = props;

    const myHandleChange = React.useCallback((event: React.ChangeEvent<HTMLInputElement>) => {
        handleChange(event);
        autoSave();
    }, [handleChange, autoSave]);

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
        <FlexBox>
            <TextField
                select
                fullWidth
                name={name}
                label={parameter.value ? parameter.name : null}
                value={parameter.value}
                onChange={myHandleChange}
                onBlur={handleBlur}
                disabled={readOnly}
                SelectProps={{
                    displayEmpty: true,
                    renderValue
                }}
            >
                {choices}
            </TextField>
            {help}
        </FlexBox>
    );
};


const XsdParameter: React.FC<XsdProps> = (props) => {
    const {parameter, readOnly, entity, actions, name, form: {handleChange, handleBlur, setFieldValue, submitForm}} = props;
    const {onEdit, onRemove} = actions.$;

    const loading = useSelector((state) => state.model.status);
    const templates = useSelector((state) => state.form.templates);
    const dispatch = useDispatch();
    const url = useParams();

    const [onSubmitted, setSubmitted] = React.useState<(() => void) | undefined>(undefined);
    const [saved, setSaved] = React.useState<boolean>(false);

    const parameter_key = React.useMemo(() => parameter.id.substr(0, parameter.id.indexOf("__template")), [parameter.id]);
    const xsd = React.useMemo(() => getXsdName(parameter_key, entity?.type), [parameter_key, entity]);

    const myHandleChange = React.useCallback((event: React.ChangeEvent<HTMLInputElement>) => {
        handleChange(event);
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
        submitForm();
    }, [dispatch, url.name, parameter_key, xsd, entity, onEdit, onRemove, handleChange, submitForm]);

    const handleClear = React.useCallback(() => {
        setFieldValue(name, "");
        onRemove && onRemove(entity?.name, parameter_key);
        submitForm();
    }, [setFieldValue, name, parameter_key, entity, onRemove, submitForm]);

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
    const hasValue = React.useMemo(() => parameter.value && parameter.value !== "", [parameter.value]);

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
        <FlexBox>
            <TextField
                select
                fullWidth
                name={name}
                label={hasValue ? parameter.name : null}
                value={templatesNames.length ? parameter.value : ""}
                onChange={myHandleChange}
                onBlur={handleBlur}
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
        </FlexBox>
    );
};


const Parameter: React.FC<Props> = (props) => {
    const {parameter, readOnly, entity, actions, prefix, form, autoSave} = props;

    const model = useSelector((state) => state.model.model);

    const isReadOnly = readOnly || parameter.readOnly;
    const name = prefix + ".value";

    if (parameter.id.endsWith("__template")) {
        return (
            <XsdParameter
                parameter={parameter}
                readOnly={isReadOnly}
                actions={actions}
                entity={entity}
                name={name}
                form={form}
            />
        );
    }

    const enumeration = model?.environment?.enums?.find(e => e.id === parameter.type);
    if (enumeration != null) {
        return (
            <EnumParam
                parameter={parameter}
                readOnly={isReadOnly}
                enumeration={enumeration.values}
                name={name}
                form={form}
                autoSave={autoSave}
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
                    form={form}
                    autoSave={autoSave}
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
                    form={form}
                    autoSave={autoSave}
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
                    name={name}
                    form={form}
                    autoSave={autoSave}
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
                    form={form}
                    autoSave={autoSave}
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
                    form={form}
                    autoSave={autoSave}
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
                    form={form}
                    autoSave={autoSave}
                />
            );
        case "string":
        case "char":
        default:
            return (
                <StringParam
                    parameter={parameter}
                    readOnly={isReadOnly}
                    name={name}
                    form={form}
                    autoSave={autoSave}
                />
            );
    }
};


interface BaseProps {
    parameter: ParameterType;
    readOnly?: boolean;
    name: string;
    form: FormikProps<any>;
    autoSave: () => void;
}


interface Props extends Omit<BaseProps, "name"> {
    entity?: {name: string; type: string;};
    prefix: string;
    actions: IActions;
}


interface NumberProps extends BaseProps {
    min: number;
    max: number;
    step: number;
}


interface EnumProps extends BaseProps {
    enumeration: string[];
}


interface XsdProps extends Omit<BaseProps, "autoSave"> {
    entity?: {name: string; type: string;};
    actions: IActions;
}


export default Parameter;
