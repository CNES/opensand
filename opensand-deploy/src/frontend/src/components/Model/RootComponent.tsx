import React from 'react';
import type {FormikProps} from 'formik';

import AppBar from '@mui/material/AppBar';
import Tab from '@mui/material/Tab';
import Tabs from '@mui/material/Tabs';
import Tooltip from '@mui/material/Tooltip';

import {styled} from '@mui/material/styles';

import Component from './Component';
import SingleListComponent from './SingleListComponent';

import {useSelector, useDispatch} from '../../redux';
import {changeTab} from '../../redux/tab';
import {noActions} from '../../utils/actions';
import type {IActions} from '../../utils/actions';
import {getComponents} from '../../xsd';
import type {Component as ComponentType} from '../../xsd';


const ColoredAppBar = styled(AppBar, {name: "ColoredAppBar", slot: "Wrapper"})({
    backgroundColor: "#FFFACD",
});


const TabPanel: React.FC<{index: any; value: any;}> = (props) => {
    const {children, value, index, ...other} = props;

    return (
        <div hidden={value !== index} {...other}>
            {value === index && children}
        </div>
    );
};


const RootComponent: React.FC<Props> = (props) => {
    const {form, xsd, autosave, actions = noActions} = props;

    const selectedTabs = useSelector((state) => state.tab);
    const visibility = useSelector((state) => state.form.visibility);
    const dispatch = useDispatch();

    const handleChange = React.useCallback((event: React.ChangeEvent<{}>, index: number) => {
        dispatch(changeTab({xsd, tab: index}));
    }, [dispatch, xsd]);

    const components = getComponents(form.values, form.values, visibility);
    const savedTab = selectedTabs[xsd];
    const value = !(savedTab && savedTab < components.length) ? 0 : savedTab;

    return (
        <React.Fragment>
            <ColoredAppBar position="static" color="primary">
                <Tabs
                    value={value}
                    onChange={handleChange}
                    indicatorColor="secondary"
                    textColor="inherit"
                    variant="fullWidth"
                >
                    {components.map(([idx, c]: [number, ComponentType], i: number) => c.description === "" ? (
                        <Tab key={c.id} label={c.name} value={i} />
                    ) : (
                        <Tooltip key={c.id} title={c.description} placement="top">
                            <Tab key={c.id} label={c.name} value={i} />
                        </Tooltip>
                    ))}
                </Tabs>
            </ColoredAppBar>
            {components.map(([idx, c]: [number, ComponentType], i: number) => {
                const elementActions = actions['#'][c.id] || noActions;
                return (
                    <TabPanel key={c.id} value={value} index={i}>
                        {c.elements.length === 1 && c.elements[0].type === "list" ? (
                            <SingleListComponent
                                list={c.elements[0].element}
                                readOnly={c.readOnly}
                                prefix={`elements.${idx}.element.elements.0.element`}
                                form={form}
                                actions={elementActions}
                                autosave={Boolean(autosave)}
                            />
                        ) : (
                            <Component
                                component={c}
                                prefix={`elements.${idx}.element`}
                                form={form}
                                actions={elementActions}
                                autosave={Boolean(autosave)}
                            />
                        )}
                    </TabPanel>
                )
            })}
        </React.Fragment>
    );
};


interface Props {
    form: FormikProps<ComponentType>;
    xsd: string;
    actions?: IActions;
    autosave?: boolean;
}


export default RootComponent;
