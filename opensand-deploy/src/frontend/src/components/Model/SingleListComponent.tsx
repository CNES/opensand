import React from 'react';

import Box from '@mui/material/Box';
import Collapse from '@mui/material/Collapse';
import IconButton from '@mui/material/IconButton';
import List from '@mui/material/List';
import ListItem from '@mui/material/ListItem';
import ListItemSecondaryAction from '@mui/material/ListItemSecondaryAction';
import ListItemText from '@mui/material/ListItemText';
import Stack from '@mui/material/Stack';

import {styled} from '@mui/material/styles';
import AddIcon from '@mui/icons-material/AddCircleOutline';
import DeleteIcon from '@mui/icons-material/HighlightOff';

import Component from './Component';

import {useListMutators} from '../../utils/hooks';
import {isReadOnly} from '../../xsd';
import type {List as ListType, Component as ComponentType} from '../../xsd';


const LeftPanel = styled(List, {name: "LeftPanel", slot: "Wrapper"})(({ theme }) => {
    const hover = theme.palette.mode === "dark" ? "rgba(223, 115, 0, .93)" : "rgba(223, 115, 0, .12)";
    const color = theme.palette.mode === "dark" ? theme.palette.common.black : theme.palette.text.primary;

    return {
        padding: 0,
        color,
        flexGrow: 1,
        '& .MuiListItem-root': {
            backgroundColor: "#FFFACD",
            textTransform: "capitalize",
            '&.Mui-selected': {
                backgroundColor: "#FFE1AC",
                '&:hover': {
                    backgroundColor: hover,
                },
            },
            '&:hover': {
                backgroundColor: hover,
            },
        },
    };
});


const SingleListComponent: React.FC<Props> = (props) => {
    const {list, readOnly, prefix} = props;

    const [open, setOpen] = React.useState<number>(0);
    const [addListItem, removeListItem] = useListMutators(list, prefix);

    const isEditable = !readOnly && !isReadOnly(list);
    const canGrow = list.elements.length < list.maxOccurences;
    const canShrink = list.elements.length > list.minOccurences;

    return (
        <Stack direction="row" spacing={0}>
            <LeftPanel>
                {isEditable && canGrow && (
                    <ListItem key={0} button selected onClick={addListItem}>
                        <ListItemText primary={`Add New ${list.pattern.name}`} />
                        <ListItemSecondaryAction>
                            <IconButton edge="end" onClick={addListItem} color="inherit">
                                <AddIcon />
                            </IconButton>
                        </ListItemSecondaryAction>
                    </ListItem>
                )}
                {list.elements.map((c: ComponentType, i: number) => (
                    <ListItem key={i+1} button selected={i === open} onClick={() => setOpen(i)}>
                        <ListItemText primary={c.name} />
                        {isEditable && canShrink && (
                            <ListItemSecondaryAction>
                                <IconButton edge="end" onClick={() => removeListItem(i)} color="inherit">
                                    <DeleteIcon />
                                </IconButton>
                            </ListItemSecondaryAction>
                        )}
                    </ListItem>
                ))}
            </LeftPanel>
            <Box flexGrow={4}>
                {list.elements.map((c: ComponentType, i: number) => (
                    <Collapse key={i} in={open === i} timeout="auto" unmountOnExit>
                        <Component
                            component={c}
                            padding={0}
                            readOnly={!isEditable}
                            prefix={`${prefix}.elements.${i}`}
                        />
                    </Collapse>
                ))}
            </Box>
        </Stack>
    );
};


interface Props {
    list: ListType;
    readOnly?: boolean;
    prefix: string;
}


export default SingleListComponent;
