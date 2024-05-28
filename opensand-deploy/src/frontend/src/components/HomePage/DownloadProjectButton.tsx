import React from 'react';

import Button from '@mui/material/Button';


const DownloadProjectButton: React.FC<Props> = (props) => {
    const {project, onClose, onClick} = props;

    React.useEffect(() => {
        if (project) {
            const form = document.createElement("form") as HTMLFormElement;
            form.method = "post";
            form.action = "/api/project/" + project;
            document.body.appendChild(form);
            form.submit();
            document.body.removeChild(form);
            onClose();
        }
    }, [project, onClose]);

    return (
        <React.Fragment>
            <Button color="secondary" variant="contained" onClick={onClick}>
                Download
            </Button>
        </React.Fragment>
    );
};


interface Props {
    project: string | false;
    onClick: () => void;
    onClose: () => void;
}


export default DownloadProjectButton;
