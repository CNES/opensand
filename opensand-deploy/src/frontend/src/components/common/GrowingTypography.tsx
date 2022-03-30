import Typography from '@mui/material/Typography';

import {styled} from '@mui/material/styles';


const GrowingTypography = styled(Typography, {name: "GrowingTypography", slot: "Wrapper"})({
    flexGrow: 1,
});


export default GrowingTypography;
