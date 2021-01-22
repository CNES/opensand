import React from 'react';
import { Link } from 'react-router-dom';
import ListItem, { ListItemProps } from '@material-ui/core/ListItem';


/*
const ListLink = (props: ListItemProps & {to: string}) => {
    const {to, ...listProps} = props;
    return (
        <Link to={to}>
            <ListItem {...listProps} />
        </Link>
    );
};
*/


// export default ListLink;
export default ListItem;
