import React from 'react';

import Button from '@material-ui/core/Button';


interface Props {
    title: string;
    onClick: () => void;
}


const CardButton = (props: Props) => {
    const {title, onClick} = props;

    const handleClick = React.useCallback((event: React.MouseEvent<HTMLButtonElement, MouseEvent>) => {
        event.stopPropagation();
        onClick();
    }, [onClick]);

    return <Button color="primary" onClick={handleClick}>{title}</Button>;
};


export default CardButton;
