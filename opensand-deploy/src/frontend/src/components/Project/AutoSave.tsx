import React from 'react';
import {useFormikContext} from 'formik';
import {isEqual} from 'lodash';

import type {Component} from '../../xsd';


const AutoSave: React.FC<Props> = (props) => {
    const {root, delay = 500} = props;
    const {values, submitForm} = useFormikContext<Component>();
    const [, setTimer] = React.useState<NodeJS.Timeout | null>(null);

    const updateTimer = React.useCallback(() => {
        const id = setTimeout(submitForm, delay);
        setTimer((old) => {
            clearTimeout(old!);
            return id;
        });
    }, [submitForm, delay]);

    React.useEffect(() => {
        if (isEqual(root, values)) {
            setTimer((old) => {clearTimeout(old!); return null;});
        } else {
            updateTimer();
        }
    }, [root, values, updateTimer]);

    return null;
};


interface Props {
    root: Component;
    delay?: number;
}


export default AutoSave;
