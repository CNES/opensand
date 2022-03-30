import React from 'react';
import {useParams, useSearchParams} from 'react-router-dom';

import Box from '@mui/material/Box';
import Typography from '@mui/material/Typography';

import {getXML, updateXML} from '../../api';
import {useSelector, useDispatch} from '../../redux';
import {clearModel} from '../../redux/model';
import {useDidMount} from '../../utils/hooks';

import Model from './Model';


const Editor: React.FC<Props> = (props) => {
    const {project, model, entity} = useParams();
    const urlFragment = React.useMemo(() => entity == null ? `${model}` : `${model}/${entity}`, [model, entity]);

    const [urlParams,] = useSearchParams();
    const xsd = React.useMemo(() => urlParams.get("xsd"), [urlParams]);
    const xml = React.useMemo(() => urlParams.get("xml"), [urlParams]);

    const dataModel = useSelector((state) => state.model.model);
    const dispatch = useDispatch();
    const didMount = useDidMount();

    const [autoSaved, setAutoSaved] = React.useState<boolean>(false);

    React.useEffect(() => {
        if (project && xsd) {
            const url = xml == null ? urlFragment : `template/${xsd}/${xml}`;
            dispatch(getXML({project, xsd, urlFragment: url}));
        }
        return () => {dispatch(clearModel());};
    }, [dispatch, project, urlFragment, xsd, xml]);

    React.useEffect(() => {
        // Weird timings involved here:
        //   Project/Project.tsx does a clearModel on unmount
        //   but we somehow end up with the project model still
        //   in dataModel on initial render.
        // So make sure to avoid this inconsistency with didMount
        // TODO: FIXME
        if (dataModel && !didMount && !autoSaved) {
            setAutoSaved(true);
            if (project && xsd && xml) {
                // Save model only if loaded from an XML template
                dispatch(updateXML({project, xsd, urlFragment}));
            }
        }
    }, [dispatch, didMount, autoSaved, dataModel, project, urlFragment, xsd, xml]);

    if (!dataModel || !xsd || !project) {
        return <Box m={3}><Typography align="center">Model does not exist!</Typography></Box>;
    }

    return (
        <Model
            model={dataModel}
            xsd={xsd}
            projectName={project}
            urlFragment={urlFragment}
        />
    );
};


interface Props {
}


export default Editor;
