import React from 'react';

import Box from '@material-ui/core/Box';
import Typography from '@material-ui/core/Typography';


const NotFound = () => {
    return (
        <Box display="flex" alignItems="center" justifyContent="center" height="100%">
            <Typography display="block" align="center" variant="h2">
                Not Found
            </Typography>
        </Box>
    );
};


export default NotFound;
