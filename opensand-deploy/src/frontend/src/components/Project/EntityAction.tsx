import React from 'react';

import CircularProgress from '@material-ui/core/CircularProgress';
import IconButton from '@material-ui/core/IconButton';
import Tooltip from '@material-ui/core/Tooltip';

import DownloadIcon from '@material-ui/icons/GetApp';
import ErrorIcon from '@material-ui/icons/Error';
import LaunchIcon from '@material-ui/icons/PlaylistPlay';
import NothingIcon from '@material-ui/icons/Cancel';
import PingIcon from '@material-ui/icons/Router';
import StopIcon from '@material-ui/icons/Stop';
import UploadIcon from '@material-ui/icons/Publish';

import {
    copyEntityConfiguration,
    deployEntity,
    findPingDestinations,
    silenceSuccess,
    IPidSuccess,
    IPingDestinations,
} from '../../api';
import {sendError} from '../../utils/dispatcher';

import {Component, isParameterElement} from '../../xsd/model';


interface Props {
    project: string;
    entity?: Component;
    onDownload: (entity?: string) => void;
    setAction: (fn?: () => (password: string, isPassphrase: boolean) => void) => void;
    setPingDestinations: (entity: string, address: string, destinations: string[]) => void;
}


const EntityAction = (props: Props) => {
    const {project, entity, onDownload, setAction, setPingDestinations} = props;

    const [disabled, setDisabled] = React.useState<boolean>(false);
    const [running, setRunning] = React.useState<boolean>(false);

    const handleCopy = React.useCallback((entity: string, folder: string) => {
        copyEntityConfiguration(silenceSuccess, sendError, project, entity, folder);
    }, [project]);

    const handleStatus = React.useCallback((entity: string, address: string, password: string, isPassphrase: boolean) => {
        return (result: IPidSuccess) => {
            setDisabled(false);
            setRunning(result.running === true);
            if (result.running) {
                const newHandler = handleStatus(entity, address, password, isPassphrase);
                const recursiveCall = () => deployEntity(newHandler, sendError, project, entity, '', '', 'STATUS', address, password, isPassphrase);
                setTimeout(recursiveCall, 5000);
            }
        };
    }, [project]);

    const handleDeploy = React.useCallback((entity: string, mode: string, folder: string, action: string, address: string, password: string, isPassphrase: boolean) => {
        const handler = handleStatus(entity, address, password, isPassphrase);
        setDisabled(true);
        deployEntity(handler, sendError, project, entity, folder, mode, action, address, password, isPassphrase);
    }, [project, handleStatus]);

    const handlePing = React.useCallback((entity: string, address: string) => {
        const callback = (result: IPingDestinations) => setPingDestinations(entity, address, result.addresses);
        findPingDestinations(callback, sendError, project);
    }, [project, setPingDestinations]);

    let title = "Error retrieving entity configuration";
    let clickAction: (() => void) | undefined = undefined;
    let child = <ErrorIcon />;

    if (running) {
        title = "OpenSAND is running";
        child = <CircularProgress size={24} />;
    }

    if (entity) {
        const entityConfig: {[key: string]: string;} = {};
        entity.elements.forEach((p) => {
            if (isParameterElement(p)) {
                entityConfig[p.element.id] = p.element.value;
            }
        });

        const {entity_name, upload, folder, run, address} = entityConfig;
        const handleAction = (password: string, isPassphrase: boolean) => (
            handleDeploy(entity_name, upload, folder, run, address, password, isPassphrase)
        );

        switch (run) {
            case "PING":
                title = "Ping the emulated network";
                child = <PingIcon />;
                clickAction = () => handlePing(entity_name, address);
                break;
            case "STOP":
                title = "Stop OpenSAND";
                child = <StopIcon />;
                clickAction = () => setAction(() => handleAction);
                break;
            case "STATUS":
                if (!running) {
                    title = "Check if OpenSAND is running";
                    child = <CircularProgress color="inherit" variant="determinate" value={30} size={24} />;
                    clickAction = () => setAction(() => handleAction);
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
                    break;
                }

                if (!folder) {
                    title = "No folder to upload into";
                    child = <UploadIcon />;
                    break;
                }

                if (!run) {
                    title = "Deploy configuration files";
                    child = <UploadIcon />;
                    clickAction = upload === "NFS" ? (
                        () => handleCopy(entity_name, folder)
                    ) : (
                        () => setAction(() => handleAction)
                    );
                    break;
                }

                title = !upload ? "Launch OpenSAND without configuration" : "Configure and launch OpenSAND";
                child = <LaunchIcon color={!upload ? "disabled" : "inherit"} />;
                clickAction = () => setAction(() => handleAction);
                break;
        }
    }

    return (
        <Tooltip title={title} placement="top">
            <span>
                <IconButton size="small" disabled={disabled || (!clickAction && !running)} onClick={clickAction}>
                    {child}
                </IconButton>
            </span>
        </Tooltip>
    );
};


export default EntityAction;
