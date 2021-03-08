import * as Model from './model';

import builder from 'xmlbuilder';


export const toXML = (model: Model.Model) => {
    const dataModel = builder.create('model', {encoding: 'UTF-8'});
    dataModel.attribute('version', model.version);
    componentToXML(model.root, dataModel);

    return dataModel.end({
        pretty: true,
        newline: '\n',
        allowEmpty: false,
    });
};


const componentToXML = (component: Model.Component, parentNode: builder.XMLElement) => {
    const componentNode = parentNode.element(component.id);
    component.parameters.forEach((p: Model.Parameter) => {if (p.value !== "") componentNode.element(p.id, {}, p.value);});
    component.children.forEach((c: Model.Component) => {componentToXML(c, componentNode);});
    component.lists.forEach((l: Model.List) => {
        const listNode = componentNode.element(l.id);
        l.elements.forEach((e: Model.Component) => {componentToXML(e, listNode);});
    });
};
