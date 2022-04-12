import React from 'react';
import {useNavigate} from 'react-router-dom';
import type {FormikProps} from 'formik';

import {useSelector} from '../redux';
import type {ThunkConfig} from '../redux';
import {clearProjects} from '../redux/projects';
import type {IAction} from '../utils/actions';
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
        setTimer(setTimeout(callback, t));
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


export const useListMutators = (list: List, actions: IAction, form: FormikProps<Component>, prefix: string) => {
    const {onCreate, onDelete} = actions;
    const {values, setFieldValue, submitForm} = form;

    const addListItem = React.useCallback(() => {
        const doAdd = (l: List, path: string, creator?: (lst: List) => Component | undefined) => {
            const item = creator ? creator(l) : newItem(l.pattern, l.elements.length);
            if (item) {
                setFieldValue(path + ".elements", [...l.elements, item]);
            }
        };
        if (onCreate != null) {
            onCreate(values, doAdd, submitForm);
        } else {
            doAdd(list, prefix);
        }
    }, [list, onCreate, setFieldValue, submitForm, values, prefix]);

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
        if (onDelete != null) {
            onDelete(values, doDelete);
            submitForm();
        } else {
            doDelete(list, prefix);
        }
    }, [list, onDelete, setFieldValue, submitForm, values, prefix]);

    return [addListItem, removeListItem] as const;
};
