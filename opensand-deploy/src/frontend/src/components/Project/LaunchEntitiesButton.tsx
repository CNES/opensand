import React from 'react';

import SpacedButton from '../common/SpacedButton';

import {deployEntity} from '../../api';
import {useDispatch} from '../../redux';
import {runSshCommand} from '../../redux/ssh';
import {isParameterElement, isComponentElement, isListElement} from '../../xsd';
import type {Component} from '../../xsd';


interface LaunchParams {
    entity: string;
    address: string;
    folder: string;
    copyMethod: string;
}


const LaunchEntitiesButton: React.FC<Props> = (props) => {
    const {project} = props;

    const dispatch = useDispatch();

    const [projectName, setProjectName] = React.useState<string>("");
    const [entities, setEntities] = React.useState<LaunchParams[]>([]);

    const handleClick = React.useCallback(() => {
        const entities: LaunchParams[] = [];

        project.elements.forEach((e) => {
            if (isComponentElement(e) && e.element.id === "platform") {
                e.element.elements.forEach((el) => {
                    if (isParameterElement(el) && el.element.id === "project") {
                        setProjectName(el.element.value);
                    } else if (isListElement(el) && el.element.id === "machines") {
                        el.element.elements.forEach((c) => {
                            const launchParams = {
                                entity: "",
                                address: "",
                                folder: "",
                                copyMethod: "",
                            };
                            c.elements.forEach((p) => {
                                if (isParameterElement(p)) {
                                    if (p.element.id === "entity_name") {
                                        launchParams.entity = p.element.value;
                                    } else if (p.element.id === "address") {
                                        launchParams.address = p.element.value;
                                    } else if (p.element.id === "folder") {
                                        launchParams.folder = p.element.value;
                                    } else if (p.element.id === "upload") {
                                        launchParams.copyMethod = p.element.value;
                                    }
                                }
                            });
                            if (Object.values(launchParams).filter((v) => !v).length === 0) {
                                entities.push(launchParams);
                            }
                        });
                    }
                });
            }
        });

        dispatch(runSshCommand({action: () => setEntities(entities)}));
    }, [dispatch, project]);

    React.useEffect(() => {
        if (entities.length) {
            entities.forEach((launchParams) => {
                dispatch(deployEntity({
                    project: projectName,
                    runMethod: "LAUNCH",
                    ...launchParams,
                }));
            });
        }
    }, [dispatch, projectName, entities]);

    return (
        <SpacedButton
            color="success"
            variant="contained"
            onClick={handleClick}
        >
            Launch All Entities
        </SpacedButton>
    );
};


interface Props {
    project: Component;
}


export default LaunchEntitiesButton;
