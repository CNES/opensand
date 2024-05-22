import React from 'react';

import Button from '@mui/material/Button';


const CardButton: React.FC<Props> = (props) => {
    const {title, onClick} = props;

    const handleClick = React.useCallback((event: React.MouseEvent<HTMLButtonElement, MouseEvent>) => {
        event.stopPropagation();
        onClick();
    }, [onClick]);

    return <Button color="primary" variant="outlined" sx={{mt: 1, width: "100%"}} style={{marginLeft: 0}} onClick={handleClick}>{title}</Button>;
};


interface Props {
    title: string;
    onClick: () => void;
}


export default CardButton;
