import React from 'react';

import {fromXSD} from '../xsd/parser';
import {Model as ModelType} from '../xsd/model';

import {getXSD, IXsdContent} from '../api';

import Model from './Model/Model';


const App = () => {
    const [model, changeModel] = React.useState<ModelType | undefined>(undefined);

    const loadModel = React.useCallback((content: IXsdContent) => {
        changeModel(fromXSD(content.content));
    }, [changeModel]);

    React.useEffect(() => {
        getXSD(loadModel, console.log, 'topology.xsd');
        return () => {changeModel(undefined);}
    }, [loadModel]);

    if (model == null) {
        return <div><p>Model not loaded</p></div>;
    }

    return <Model model={model} />;
};


export default App;
