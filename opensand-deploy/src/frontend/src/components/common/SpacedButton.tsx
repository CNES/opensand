import Button from '@mui/material/Button';

import {styled} from '@mui/material/styles';


const SpacedButton = styled(Button, {name: "SpacedButton", slot: "Wrapper"})(({ theme }) => ({
    marginRight: theme.spacing(2),
}));


export default SpacedButton;
