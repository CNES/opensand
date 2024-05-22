import React from 'react';

import Avatar from '@mui/material/Avatar';
import Card from '@mui/material/Card';
import CardActions from '@mui/material/CardActions';
import CardHeader from '@mui/material/CardHeader';
import CardMedia from '@mui/material/CardMedia';

import AddIcon from '@mui/icons-material/NoteAdd';


const CardAdd: React.FC<React.PropsWithChildren<Props>> = ({title, subtitle, children}) => {
    return (
        <Card variant="outlined">
            <CardHeader
                avatar={<Avatar><AddIcon /></Avatar>}
                title={`Add a new ${title}`}
                subheader={subtitle}
            />
            <CardMedia
                component="img"
                image={process.env.PUBLIC_URL + '/assets/add.jpg'}
                alt="Add icon"
                height="180"
                sx={{objectFit: "contain"}}
            />
            <CardActions>
                {children}
            </CardActions>
        </Card>
    );
};


interface Props {
    title: string;
    subtitle?: string;
}


export default CardAdd;
