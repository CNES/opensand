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

    const [running, setRunning] = React.useState<boolean>(false);

    const handleCopy = React.useCallback((entity: string, folder: string) => {
        copyEntityConfiguration(silenceSuccess, sendError, project, entity, folder);
    }, [project]);

    const handleStatus = React.useCallback((entity: string, address: string, password: string, isPassphrase: boolean) => {
        return (result: IPidSuccess) => {
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
        deployEntity(handler, sendError, project, entity, folder, mode, action, address, password, isPassphrase);
    }, [project, handleStatus]);

    const handlePing = React.useCallback((entity: string, address: string) => {
        const callback = (result: IPingDestinations) => setPingDestinations(entity, address, result.addresses);
        findPingDestinations(callback, sendError, project);
    }, [project, setPingDestinations]);

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
                return (
                    <Tooltip title="Ping the emulated network" placement="top">
                        <IconButton size="small" onClick={() => handlePing(entity_name, address)}>
                            <PingIcon />
                        </IconButton>
                    </Tooltip>
                );
            case "STOP":
                return (
                    <Tooltip title="Stop OpenSAND" placement="top">
                        <IconButton size="small" onClick={() => setAction(() => handleAction)}>
                            <StopIcon />
                        </IconButton>
                    </Tooltip>
                );
            case "STATUS":
                return (
                    <Tooltip title="Check if OpenSAND is running" placement="top">
                        <IconButton size="small" onClick={() => setAction(() => handleAction)}>
                            <CircularProgress color="inherit" variant="determinate" value={30} size={24} />
                        </IconButton>
                    </Tooltip>
                );
            default:
                if (upload === "Download") {
                    return (
                        <Tooltip title="Download configuration files" placement="top">
                            <IconButton size="small" onClick={() => onDownload(entityConfig.entity_name)}>
                                <DownloadIcon />
                            </IconButton>
                        </Tooltip>
                    );
                }

                const noUpload = Boolean(upload);
                const noRun = Boolean(run);
                if (noUpload && noRun) {
                    return (
                        <Tooltip title="No action configured for this entity" placement="top">
                            <span>
                                <IconButton size="small" disabled>
                                    <NothingIcon />
                                </IconButton>
                            </span>
                        </Tooltip>
                    );
                }

                if (Boolean(folder)) {
                    return (
                        <Tooltip title="No folder to upload into" placement="top">
                            <span>
                                <IconButton size="small" disabled>
                                    <UploadIcon />
                                </IconButton>
                            </span>
                        </Tooltip>
                    );
                }

                if (noRun) {
                    const handleClick = upload === "NFS" ? (
                        () => handleCopy(entity_name, folder)
                    ) : (
                        () => setAction(() => handleAction)
                    );
                    return (
                        <Tooltip title="Deploy configuration files" placement="top">
                            <IconButton size="small" onClick={handleClick}>
                                <UploadIcon />
                            </IconButton>
                        </Tooltip>
                    );
                }

                if (running) {
                    return (
                        <Tooltip title="OpenSAND is running" placement="top">
                            <CircularProgress size={24} />
                        </Tooltip>
                    );
                }

                const title = noUpload ? "Launch OpenSAND without configuration" : "Configure and launch OpenSAND";
                return (
                    <Tooltip title={title} placement="top">
                        <IconButton size="small" onClick={() => setAction(() => handleAction)}>
                            <LaunchIcon color={noUpload ? "disabled" : "inherit"} />
                        </IconButton>
                    </Tooltip>
                );
        }
    }

    return (
        <Tooltip title="Error retrieving entity configuration" placement="top">
            <span>
                <IconButton size="small" disabled>
                    <ErrorIcon />
                </IconButton>
            </span>
        </Tooltip>
    );
};


export default EntityAction;
