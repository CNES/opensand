import React from 'react';

import FormControl from '@material-ui/core/FormControl';
import FormHelperText from '@material-ui/core/FormHelperText';
import Divider from '@material-ui/core/Divider';
import IconButton from '@material-ui/core/IconButton';
import InputLabel from '@material-ui/core/InputLabel';
import ListSubheader from '@material-ui/core/ListSubheader';
import MenuItem from '@material-ui/core/MenuItem';
import Select from '@material-ui/core/Select';
import TextField from '@material-ui/core/TextField';
import Tooltip from '@material-ui/core/Tooltip';

import DeleteIcon from '@material-ui/icons/HighlightOff';
import EditIcon from '@material-ui/icons/Edit';

import {ITemplatesContent} from '../../api';
import {parameterStyles} from '../../utils/theme';
import {useDidMount} from '../../utils/hooks';
import {Parameter, Enum} from '../../xsd/model';


interface Props {
    parameter: Parameter;
    templates: ITemplatesContent;
    onSelect?: () => void;
    onEdit: (model: string, xsd: string, xml?: string) => void;
    onDelete: (model: string) => void;
    enumeration?: Enum;
    entityType?: string;
}


interface Option {
    xsd: string;
    xml?: string;
}


const xsdFromType = (entityType?: string) => {
    switch (entityType) {
        case "Gateway":
            return "profile_gw.xsd";
        case "Gateway Net Access":
            return "profile_gw_net_acc.xsd";
        case "Gateway Phy":
            return "profile_gw_phy.xsd";
        case "Terminal":
            return "profile_st.xsd";
        default:
            return "";
    }
}


const ProjectParameter = (props: Props) => {
    const {parameter, templates, enumeration, onSelect, onEdit, onDelete, entityType} = props;
    const didMount = useDidMount();
    const classes = parameterStyles();

    const handleChange = React.useCallback((event: React.ChangeEvent<{name?: string; value: Option;}>) => {
        const {value} = event.target;
        if (value == null || !value.hasOwnProperty("xsd")) {
            return;
        }

        const {xsd, xml} = value;
        parameter.value = xsd;
        onEdit(parameter.id, xsd, xml);
    }, [parameter, onEdit]);

    const handleClear = React.useCallback(() => {
        parameter.value = "";
        onDelete(parameter.id);
    }, [parameter, onDelete]);

    const handleEdit = React.useCallback(() => {
        onEdit(parameter.id, parameter.value);
    }, [parameter, onEdit]);

    const header = React.useMemo(() => <em>Please select a {parameter.name}</em>, [parameter]);

    const renderValue = React.useCallback((selected: any) => {
        if (selected == null || selected === "") {
            return header;
        } else {
            return selected;
        }
    }, [header]);

    React.useEffect(() => {
        if (didMount && onSelect) onSelect();
    }, [onSelect, didMount]);

    if (enumeration == null) {
        return (
            <div className={classes.spaced}>
                <TextField
                    variant="outlined"
                    label={parameter.name}
                    value={parameter.value}
                    fullWidth
                    disabled
                />
            </div>
        );
    }

    const templatesNames = enumeration.values.filter((v:string) => v !== "");
    const templateMapping: [string, string[]][] = templatesNames.map((v: string) => (
        [v, [...(templates[v] || [])]]
    ));
    const cumulativeSum = ((sum: number) => (
        ([name, templateNames]: [string, string[]]) => {
            const old = sum;
            sum += templateNames.length + 1;
            return old;
        })
    )(templateMapping.length + 4);
    const offsets = templateMapping.map(cumulativeSum);

    const choiceTemplates = templateMapping.map(([name, templateNames]: [string, string[]], i: number) => {
        const offset = offsets[i];
        const choices = templateNames.map((n: string, j: number) => (
            <MenuItem
                // @ts-ignore [1]
                value={{xsd: name, xml: n}}
                key={offset + j + 1}
            >
                {n}
            </MenuItem>
        ));
        return [<ListSubheader key={offset}>{name}</ListSubheader>, ...choices];
    });
    const choiceModels = templateMapping.map(([name, templateNames]: [string, string[]], i: number) => (
        <MenuItem
            // @ts-ignore [1]
            value={{xsd: name}}
            key={i + 2}
        >
            {name}
        </MenuItem>
    ));
    const dividers = [
        <Divider key={choiceModels.length + 2} />,
        <ListSubheader key={choiceModels.length + 3}>From Template</ListSubheader>,
    ];
    const choices = [
        <MenuItem value="" key={0}>{header}</MenuItem>,
        <ListSubheader key={1}>Models</ListSubheader>,
    ].concat(choiceModels, dividers, ...choiceTemplates);

    const disabled = parameter.value != null && parameter.value !== "";
    let error: string | undefined = undefined;
    if (disabled) {
        const expectedType = xsdFromType(entityType);
        if (!entityType && parameter.id === "infrastructure") {
            error = "No entity type selected on this configuration file";
        } else if (parameter.id === "profile" && expectedType !== parameter.value) {
            error = `Profile ${parameter.value} selected but entity type ${entityType} expects ${expectedType}`;
        }
    }

    return (
        <div className={classes.spaced}>
            <FormControl className={classes.fullWidth} error={Boolean(error)}>
                <InputLabel htmlFor={parameter.id}>
                    {parameter.value === "" ? null : parameter.name}
                </InputLabel>
                <Select
                    value={parameter.value}
                    onChange={handleChange as (event: React.ChangeEvent<{name?: string; value: unknown;}>) => void}
                    inputProps={{id: parameter.id}}
                    displayEmpty
                    renderValue={renderValue}
                    disabled={disabled}
                >
                    {choices}
                </Select>
                {error && <FormHelperText>{error}</FormHelperText>}
            </FormControl>
            {disabled && (
                <Tooltip placement="top" title="Edit this configuration file">
                    <IconButton onClick={handleEdit}>
                        <EditIcon />
                    </IconButton>
                </Tooltip>
            )}
            {disabled && (
                <Tooltip placement="top" title="Remove this configuration file">
                    <IconButton onClick={handleClear}>
                        <DeleteIcon />
                    </IconButton>
                </Tooltip>
            )}
        </div>
    );
};


export default ProjectParameter;