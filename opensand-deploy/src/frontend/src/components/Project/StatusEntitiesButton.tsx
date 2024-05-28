import React from 'react';

import Button from '@mui/material/Button';

import {deployEntity} from '../../api';
import {useDispatch, useSelector} from '../../redux';
import {runSshCommand} from '../../redux/ssh';
import {isParameterElement, isComponentElement, isListElement} from '../../xsd';
import type {Component} from '../../xsd';


const StatusEntitiesButton: React.FC<Props> = (props) => {
    const {project} = props;

    const sshConfigured = useSelector((state) => state.ssh.configured);
    const dispatch = useDispatch();

    const [didRan, setDidRan] = React.useState<boolean>(false);

    const handleClick = React.useCallback(() => {
        let projectName = "";
        const entities: [string, string][] = [];

        project.elements.forEach((e) => {
            if (isComponentElement(e) && e.element.id === "platform") {
                e.element.elements.forEach((el) => {
                    if (isParameterElement(el) && el.element.id === "project") {
                        projectName = el.element.value;
                    } else if (isListElement(el) && el.element.id === "machines") {
                        el.element.elements.forEach((c) => {
                            let name = "";
                            let address = "";
                            c.elements.forEach((p) => {
                                if (isParameterElement(p)) {
                                    if (p.element.id === "entity_name") {
                                        name = p.element.value;
                                    } else if (p.element.id === "address") {
                                        address = p.element.value;
                                    }
                                }
                            });
                            if (name && address) {
                                entities.push([name, address]);
                            }
                        });
                    }
                });
            }
        });

        setDidRan(true);
        dispatch(runSshCommand({
            action: () => {
                entities.forEach(([entity, address]) => {
                    dispatch(deployEntity({
                        project: projectName,
                        entity, address,
                        runMethod: "STATUS",
                        folder: "", copyMethod: "",
                    }));
                });
            },
        }));
    }, [dispatch, project]);

    React.useEffect(() => {
        if (!didRan && sshConfigured) {
            setDidRan(true);
            // handleClick();
        }
    }, [didRan, sshConfigured, handleClick]);

    return (
        <Button
            color="secondary"
            variant="contained"
            onClick={handleClick}
        >
            Status of All Entities
        </Button>
    );
};


interface Props {
    project: Component;
}


export default StatusEntitiesButton;
