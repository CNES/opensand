import React from 'react';

import AppBar from '@material-ui/core/AppBar';
import Tab from '@material-ui/core/Tab';
import Tabs from '@material-ui/core/Tabs';
import Tooltip from '@material-ui/core/Tooltip';

import Component from './Component';
import SingleListComponent from './SingleListComponent';

import {Component as ComponentType} from '../../xsd/model';


interface Props {
    root: ComponentType;
    modelChanged: () => void;
}


interface PanelProps {
    className?: string;
    children?: React.ReactNode;
    index: any;
    value: any;
}


const TabPanel = (props: PanelProps) => {
    const {children, value, index, ...other} = props;

    return (
        <div hidden={value !== index} {...other}>
            {value === index && children}
        </div>
    );
};


const RootComponent = (props: Props) => {
    const {root, modelChanged} = props;

    const [value, setValue] = React.useState<number>(0);

    const handleChange = React.useCallback((event: React.ChangeEvent<{}>, index: number) => {
        setValue(index);
    }, [setValue]);

    const components = root.getComponents();

    return (
        <React.Fragment>
            <AppBar position="static" color="primary">
                <Tabs
                    value={value}
                    onChange={handleChange}
                    indicatorColor="secondary"
                    textColor="inherit"
                    variant="fullWidth"
                >
                    {components.map((c: ComponentType, i: number) => c.description === "" ? (
                        <Tab key={c.id} label={c.name} value={i} />
                    ) : (
                        <Tooltip title={c.description} placement="top" key={c.id}>
                            <Tab key={c.id} label={c.name} value={i} />
                        </Tooltip>
                    ))}
                </Tabs>
            </AppBar>
            {components.map((c: ComponentType, i: number) => (
                <TabPanel key={i} value={value} index={i}>
                    {c.elements.length === 1 && c.elements[0].type === "list" ? (
                        <SingleListComponent
                            list={c.elements[0].element}
                            readOnly={c.readOnly}
                            changeModel={modelChanged}
                        />
                    ) : (
                        <Component component={c} changeModel={modelChanged} />
                    )}
                </TabPanel>
            ))}
        </React.Fragment>
    );
};


export default RootComponent;
