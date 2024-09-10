import React from 'react';
import {useNavigate} from 'react-router-dom';
import {useFormikContext} from 'formik';

import {useSelector} from '../redux';
import type {ThunkConfig} from '../redux';
import {clearProjects} from '../redux/projects';
import type {IProjectAction} from '../utils/actions';
import {newItem} from '../xsd';
import type {List, Component} from '../xsd';


export const useDidMount = () => {
    const didMountRef = React.useRef<boolean>(true);

    React.useEffect(() => {
        didMountRef.current = false;
    }, [didMountRef]);

    return didMountRef.current;
};


export const useOpen = (initialState: boolean = false) => {
    const [open, setOpen] = React.useState<boolean>(initialState);

    const handleOpen = React.useCallback(() => {setOpen(true);}, []);
    const handleClose = React.useCallback(() => {setOpen(false);}, []);

    return [open, handleOpen, handleClose] as const;
};


export const useFormSubmit = () => {
    const saved = useSelector((state) => state.model.status);

    const [finished, onFinished] = React.useState<(() => void) | undefined>(undefined);

    React.useEffect(() => {
        if (finished && (saved === "success" || saved === "error")) {
            finished();
            onFinished(undefined);
        }
    }, [saved, finished]);

    return onFinished;
};


declare type Callback = () => void;
export const useTimer = (timeout: number) => {
    const [timer, setTimer] = React.useState<NodeJS.Timeout | null>(null);

    const changeTimer = React.useCallback((callback: Callback, t: number = timeout) => {
        const id = setTimeout(callback, t);
        setTimer(id);
    }, [timeout]);

    React.useEffect(() => {
        if (timer != null) {
            return () => {
                clearTimeout(timer);
            }
        }
    }, [timer]);

    return changeTimer;
};


declare type Dispatch = Pick<ThunkConfig, "dispatch">["dispatch"];
export const useProject = (dispatch: Dispatch) => {
    const navigate = useNavigate();

    const goToProject = React.useCallback((projectName: string) => {
        dispatch(clearProjects());
        navigate("/project/" + projectName);
    }, [dispatch, navigate]);

    return goToProject;
};


export const useProjectMutators = (actions: IProjectAction) => {
    const {onCreate, onDelete} = actions;
    const {setFieldValue, submitForm} = useFormikContext<Component>();

    const addListItem = React.useCallback(() => {
        const doAdd = (l: List, path: string, creator: (lst: List) => Component | undefined) => {
            const item = creator(l);
            if (item) {
                setFieldValue(path + ".elements", [...l.elements, item]);
            }
        };
        onCreate(doAdd, submitForm);
    }, [onCreate, setFieldValue, submitForm]);

    const removeListItem = React.useCallback((index: number) => {
        const doDelete = (l: List, path: string) => {
            const fieldValues = l.elements.slice(index, -1);
            const newValues = [...l.elements];
            newValues.splice(index, 1);
            fieldValues.forEach((c: Component, i: number) => {
                const oldComponent = newValues[index + i];
                newValues[index + i] = {...oldComponent, refPath: c.refPath, name: c.name};
            });
            setFieldValue(path + ".elements", newValues);
        };
        onDelete(doDelete);
        submitForm();
    }, [onDelete, setFieldValue, submitForm]);

    return [addListItem, removeListItem] as const;
};


export const useListMutators = (list: List, prefix: string) => {
    const {setFieldValue} = useFormikContext<Component>();

    const addListItem = React.useCallback(() => {
        const item = newItem(list.pattern, list.elements.length);
        if (item) {
            setFieldValue(prefix + ".elements", [...list.elements, item]);
        }
    }, [list, setFieldValue, prefix]);

    const removeListItem = React.useCallback((index: number) => {
        const fieldValues = list.elements.slice(index, -1);
        const newValues = [...list.elements];
        newValues.splice(index, 1);
        fieldValues.forEach((c: Component, i: number) => {
            const oldComponent = newValues[index + i];
            newValues[index + i] = {...oldComponent, refPath: c.refPath, name: c.name};
        });
        setFieldValue(prefix + ".elements", newValues);
    }, [list, setFieldValue, prefix]);

    return [addListItem, removeListItem] as const;
};
