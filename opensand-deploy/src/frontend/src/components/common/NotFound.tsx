import React from 'react';

import Box from '@mui/material/Box';
import Typography from '@mui/material/Typography';


const NotFound: React.FC<Props> = (props) => {
    return (
        <Box display="flex" alignItems="center" justifyContent="center" height="100%">
            <Typography display="block" align="center" variant="h2">
                Not Found
            </Typography>
        </Box>
    );
};


interface Props {
}


export default NotFound;
