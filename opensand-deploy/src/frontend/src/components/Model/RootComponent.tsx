import React from 'react';
import {useFormikContext} from 'formik';

import Box from '@mui/material/Box';
import Tab from '@mui/material/Tab';
import Tabs from '@mui/material/Tabs';
import Tooltip from '@mui/material/Tooltip';

import Component from './Component';
import SingleListComponent from './SingleListComponent';

import {useSelector, useDispatch} from '../../redux';
import {changeTab} from '../../redux/tab';
import {getComponents} from '../../xsd';
import type {Component as ComponentType} from '../../xsd';


const TabPanel: React.FC<React.PropsWithChildren<{index: any; value: any;}>> = (props) => {
    const {children, value, index} = props;

    return (
        <Box hidden={value !== index}>
            {value === index && children}
        </Box>
    );
};


const RootComponent: React.FC<Props> = (props) => {
    const {xsd, root} = props;
    const {values} = useFormikContext<ComponentType>();

    const selectedTabs = useSelector((state) => state.tab);
    const visibility = useSelector((state) => state.form.visibility);
    const dispatch = useDispatch();

    const handleChange = React.useCallback((event: React.ChangeEvent<{}>, index: number) => {
        dispatch(changeTab({xsd, tab: index}));
    }, [dispatch, xsd]);

    const components = getComponents(root, values, visibility);
    const savedTab = selectedTabs[xsd];
    const value = !(savedTab && savedTab < components.length) ? 0 : savedTab;

    return (
        <React.Fragment>
            <Box sx={{backgroundColor: "#FFFACD", borderBottom: 1, borderColor: "divider"}}>
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
            </Box>
            {components.map(([idx, c]: [number, ComponentType], i: number) => {
                return (
                    <TabPanel key={c.id} value={value} index={i}>
                        {c.elements.length === 1 && c.elements[0].type === "list" ? (
                            <SingleListComponent
                                list={c.elements[0].element}
                                readOnly={c.readOnly}
                                prefix={`elements.${idx}.element.elements.0.element`}
                            />
                        ) : (
                            <Component
                                component={c}
                                prefix={`elements.${idx}.element`}
                            />
                        )}
                    </TabPanel>
                )
            })}
        </React.Fragment>
    );
};


interface Props {
    xsd: string;
    root: ComponentType;
}


export default RootComponent;
