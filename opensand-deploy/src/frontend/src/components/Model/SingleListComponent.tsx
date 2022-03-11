import React from 'react';
import type {FormikProps} from 'formik';

import Collapse from '@mui/material/Collapse';
import IconButton from '@mui/material/IconButton';
import List from '@mui/material/List';
import ListItem from '@mui/material/ListItem';
import ListItemIcon from '@mui/material/ListItemIcon';
import ListItemSecondaryAction from '@mui/material/ListItemSecondaryAction';
import ListItemText from '@mui/material/ListItemText';

import {styled} from '@mui/material/styles';
import AddIcon from '@mui/icons-material/AddCircleOutline';
import DeleteIcon from '@mui/icons-material/HighlightOff';

import Component from './Component';

import {noActions} from '../../utils/actions';
import type {IActions} from '../../utils/actions';
import {useListMutators} from '../../utils/hooks';
import {isReadOnly} from '../../xsd';
import type {List as ListType, Component as ComponentType} from '../../xsd';


const FlexBox = styled('div')({
    display: "flex",
});


const RightPanel = styled('div')({
    flexGrow: 4,
});


const LeftPanel = styled(List, {name: "LeftPanel", slot: "Wrapper"})(({ theme }) => ({
    color: theme.palette.mode === "dark" ? theme.palette.common.black : theme.palette.text.primary,
    flexGrow: 1,
    '& .MuiListItem-root': {
        backgroundColor: "#FFFACD",
        textTransform: "capitalize",
        '&.Mui-selected': {
            backgroundColor: "#FFE1AC",
        },
    },
}));


const SingleListComponent: React.FC<Props> = (props) => {
    const {list, readOnly, prefix, form, autoSave, actions} = props;

    const [open, setOpen] = React.useState<number>(0);
    const [addListItem, removeListItem] = useListMutators(list, actions.$, form, prefix);

    const isEditable = !readOnly && !isReadOnly(list);
    const canGrow = list.elements.length < list.maxOccurences;
    const canShrink = list.elements.length > list.minOccurences;

    return (
        <FlexBox>
            <LeftPanel>
                {isEditable && canGrow && (
                    <ListItem key={0} button selected onClick={addListItem}>
                        <ListItemIcon><AddIcon /></ListItemIcon>
                        <ListItemText primary={`Add New ${list.pattern.name}`} />
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
            <RightPanel>
                {list.elements.map((c: ComponentType, i: number) => (
                    <Collapse key={i} in={open === i} timeout="auto" unmountOnExit>
                        <Component
                            component={c}
                            readOnly={!isEditable}
                            prefix={`${prefix}.elements.${i}`}
                            form={form}
                            actions={actions['#'][c.id] || noActions}
                            autoSave={autoSave}
                        />
                    </Collapse>
                ))}
            </RightPanel>
        </FlexBox>
    );
};


interface Props {
    list: ListType;
    readOnly?: boolean;
    prefix: string;
    form: FormikProps<ComponentType>;
    actions: IActions;
    autoSave: () => void;
}


export default SingleListComponent;
