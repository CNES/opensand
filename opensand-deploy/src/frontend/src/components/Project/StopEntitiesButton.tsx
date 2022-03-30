import React from 'react';

import SpacedButton from '../common/SpacedButton';

import {deployEntity} from '../../api';
import {useDispatch} from '../../redux';
import {runSshCommand} from '../../redux/ssh';
import {isParameterElement, isComponentElement, isListElement} from '../../xsd';
import type {Component} from '../../xsd';


interface StopParams {
    entity: string;
    address: string;
}


const StopEntitiesButton: React.FC<Props> = (props) => {
    const {project} = props;

    const dispatch = useDispatch();

    const [projectName, setProjectName] = React.useState<string>("");
    const [entities, setEntities] = React.useState<StopParams[]>([]);

    const handleClick = React.useCallback(() => {
        const entities: StopParams[] = [];

        project.elements.forEach((e) => {
            if (isComponentElement(e) && e.element.id === "platform") {
                e.element.elements.forEach((el) => {
                    if (isParameterElement(el) && el.element.id === "project") {
                        setProjectName(el.element.value);
                    } else if (isListElement(el) && el.element.id === "machines") {
                        el.element.elements.forEach((c) => {
                            const stopParams = {entity: "", address: ""};
                            c.elements.forEach((p) => {
                                if (isParameterElement(p)) {
                                    if (p.element.id === "entity_name") {
                                        stopParams.entity = p.element.value;
                                    } else if (p.element.id === "address") {
                                        stopParams.address = p.element.value;
                                    }
                                }
                            });
                            if (Object.values(stopParams).filter((v) => !v).length === 0) {
                                entities.push(stopParams);
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
            setEntities([]);
            entities.forEach((stopParams) => {
                dispatch(deployEntity({
                    project: projectName,
                    runMethod: "STOP",
                    folder: "", copyMethod: "",
                    ...stopParams,
                }));
            });
        }
    }, [dispatch, projectName, entities]);

    return (
        <SpacedButton
            color="error"
            variant="contained"
            onClick={handleClick}
        >
            Stop All Entities
        </SpacedButton>
    );
};


interface Props {
    project: Component;
}


export default StopEntitiesButton;
