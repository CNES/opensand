import React from 'react';
import { Link } from 'react-router-dom';

import Tab, { TabProps } from '@material-ui/core/Tab';


interface TabLinkProps extends TabProps {
    to: string;
}

const TabLink = (props: TabLinkProps) => (
    <Tab {...props} component={ Link as any } />
)


export default TabLink;