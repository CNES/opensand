import React from 'react';


export const useDidMount = () => {
    const didMountRef = React.useRef<boolean>(true);

    React.useEffect(() => {
        didMountRef.current = false;
    }, [didMountRef]);

    return didMountRef.current;
};
