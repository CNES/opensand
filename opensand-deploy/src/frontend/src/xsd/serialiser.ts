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
    component.elements.forEach((e: Model.Element) => {
        switch (e.type) {
            case "parameter":
                const p: Model.Parameter = e.element;
                if (p.value !== "") {
                    componentNode.element(p.id, {}, p.value);
                }
                break;
            case "component":
                const c: Model.Component = e.element;
                componentToXML(c, componentNode);
                break;
            case "list":
                const l: Model.List = e.element;
                const listNode = componentNode.element(l.id);
                l.elements.forEach((item: Model.Component) => {componentToXML(item, listNode);});
                break;
        }
    });
};
