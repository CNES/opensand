import React from 'react';
import { Link } from 'react-router-dom';

import Button, { ButtonProps } from '@material-ui/core/Button';


interface ButtonLinkProps extends ButtonProps {
    to: string;
}

const ButtonLink = (props: ButtonLinkProps) => (
    <Button {...props} component={ Link as any } />
)


export default ButtonLink;