import React from 'react';

import CircularProgress from '@mui/material/CircularProgress';
import IconButton from '@mui/material/IconButton';
import Tooltip from '@mui/material/Tooltip';

import DownloadIcon from '@mui/icons-material/GetApp';
import ErrorIcon from '@mui/icons-material/Error';
import LaunchIcon from '@mui/icons-material/PlaylistPlay';
import NothingIcon from '@mui/icons-material/Cancel';
import PingIcon from '@mui/icons-material/Router';
import StopIcon from '@mui/icons-material/Stop';
import UploadIcon from '@mui/icons-material/Publish';

import {copyEntityConfiguration, deployEntity, findPingDestinations} from '../../api';
import type {DeployParameters} from '../../api';
import {useSelector, useDispatch} from '../../redux';
import {createEntityAction} from '../../redux/action';
import {setPingingEntity} from '../../redux/ping';
import {runSshCommand} from '../../redux/ssh';
import {isParameterElement} from '../../xsd';
import type {Component} from '../../xsd';


const EntityAction: React.FC<Props> = (props) => {
    const {project, entity, onDownload} = props;

    const sshConfigured = useSelector((state) => state.ssh.configured);
    const state = useSelector((state) => state.action);
    const dispatch = useDispatch();

    const [deployParameters, setDeployParameters] = React.useState<DeployParameters | null>(null);

    const handleCopy = React.useCallback((entity: string, folder: string) => {
        if (project) {
            dispatch(copyEntityConfiguration({project, entity, folder}));
        }
    }, [dispatch, project]);

    const handleDeploy = React.useCallback((entity: string, mode: string, folder: string, action: string, address: string) => {
        if (project) {
            dispatch(runSshCommand({
                action: () => setDeployParameters({
                    project, entity, address,
                    folder, copyMethod: mode,
                    runMethod: action,
                })
            }));
        }
    }, [dispatch, project]);

    const handlePing = React.useCallback((entity: string, address: string) => {
        if (project) {
            dispatch(findPingDestinations({project}));
            dispatch(setPingingEntity({name: entity, address}));
        }
    }, [dispatch, project]);

    React.useEffect(() => {
        if (entity) {
            const entityName = entity.elements.find((p) => (
                isParameterElement(p) && p.element.id === "entity_name"
            ));
            if (isParameterElement(entityName)) {
                const name = entityName.element.value;
                const config = state[name];
                if (!config) {
                    dispatch(createEntityAction(name));
                }
            }
        }
    }, [dispatch, state, entity]);

    React.useEffect(() => {
        if (project && entity && sshConfigured) {
            const entityName = entity.elements.find((p) => (
                isParameterElement(p) && p.element.id === "entity_name"
            ));
            const entityAddress = entity.elements.find((p) => (
                isParameterElement(p) && p.element.id === "address"
            ));
            if (isParameterElement(entityName) && isParameterElement(entityAddress)) {
                setDeployParameters({
                    project, folder: "", copyMethod: "",
                    entity: entityName.element.value,
                    address: entityAddress.element.value,
                    runMethod: "STATUS",
                });
            }
        }
    }, [sshConfigured, entity, project, dispatch]);

    React.useEffect(() => {
        if (deployParameters) {
            setDeployParameters(null);
            dispatch(deployEntity(deployParameters));
        }
    }, [dispatch, deployParameters]);

    let title = "Error retrieving entity configuration";
    let clickAction: (() => void) | undefined = undefined;
    let child = <ErrorIcon />;
    let disabled = true;

    if (entity) {
        const entityConfig: {[key: string]: string;} = {};
        entity.elements.forEach((p) => {
            if (isParameterElement(p)) {
                entityConfig[p.element.id] = p.element.value;
            }
        });

        const {entity_name, upload, folder, run, address} = entityConfig;
        const handleAction = () => handleDeploy(entity_name, upload, folder, run, address);

        const entity_config = state[entity_name];
        if (!entity_config) {
            return null;
        }

        const running = entity_config.running;
        disabled = !running && entity_config.status === "pending";

        if (running) {
            title = "OpenSAND is running";
            child = <CircularProgress size={24} />;
        }

        switch (run) {
            case "PING":
                title = "Ping the emulated network";
                child = <PingIcon />;
                clickAction = () => handlePing(entity_name, address);
                break;
            case "STOP":
                title = "Stop OpenSAND";
                child = <StopIcon />;
                clickAction = handleAction;
                break;
            case "STATUS":
                if (!running) {
                    title = "Check if OpenSAND is running";
                    child = <CircularProgress color="inherit" variant="determinate" value={30} size={24} />;
                    clickAction = handleAction;
                }
                break;
            default:
                if (upload === "Download") {
                    title = "Download configuration files";
                    child = <DownloadIcon />;
                    clickAction = () => onDownload(entity_name);
                    break;
                }

                if (running) {
                    break;
                }

                if (!upload && !run) {
                    title = "No action configured for this entity";
                    child = <NothingIcon />;
                    disabled = true;
                    break;
                }

                if (!folder) {
                    title = "No folder to upload into";
                    child = <UploadIcon />;
                    disabled = true;
                    break;
                }

                if (!run) {
                    title = "Deploy configuration files";
                    child = <UploadIcon />;
                    clickAction = upload === "NFS" ? (() => handleCopy(entity_name, folder)) : handleAction;
                    break;
                }

                title = !upload ? "Launch OpenSAND without configuration" : "Configure and launch OpenSAND";
                child = <LaunchIcon color={!upload ? "disabled" : "inherit"} />;
                clickAction = handleAction;
                break;
        }
    }

    return (
        <Tooltip title={title} placement="top">
            <span>
                <IconButton size="small" disabled={disabled} onClick={clickAction}>
                    {child}
                </IconButton>
            </span>
        </Tooltip>
    );
};


interface Props {
    project?: string;
    entity?: Component;
    onDownload: (entity?: string) => void;
}


export default EntityAction;
