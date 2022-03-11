import React from 'react';

import Button from '@mui/material/Button';


const CardButton: React.FC<Props> = (props) => {
    const {title, onClick} = props;

    const handleClick = React.useCallback((event: React.MouseEvent<HTMLButtonElement, MouseEvent>) => {
        event.stopPropagation();
        onClick();
    }, [onClick]);

    return <Button color="primary" onClick={handleClick}>{title}</Button>;
};


interface Props {
    title: string;
    onClick: () => void;
}


export default CardButton;
