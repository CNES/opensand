import * as Model from './model';

import {parseString as parseXSD} from 'xml2js';


interface Documentation {
    "$": {"xml:lang": string;};
    advanced: string[];
    description: string[];
    expected: string[];
    name: string[];
    reference: string[];
    type: string[];
    unit: string[];
}


interface Annotation {
    "xs:documentation": Documentation[];
}


interface Parameter {
    "$": {name: string; type: string};
    "xs:annotation": Annotation[];
}


interface Component {
    "$": {name: string;};
    "xs:annotation": Annotation[];
    "xs:complexType": SubComponent[];
}


interface Pattern extends Component {
    "$": {name: string; minOccurs: string; maxOccurs: "unbounded"};
}


type Element = Parameter | Component | Pattern;


interface SubComponent {
    "xs:sequence": Sequence[];
}


interface Sequence {
    "xs:element": Element[];
}


interface ModelDocumentation {
    "$": {"xml:lang": string;};
    version: string[];
}


interface ModelAnnotation {
    "xs:documentation": ModelDocumentation[];
}


interface RootSequence {
    "xs:element": Component[];
}


interface RootComponent {
    "xs:sequence": RootSequence[];
}


interface ModelComponent {
    "$": {name: string;};
    "xs:annotation": ModelAnnotation[];
    "xs:complexType": RootComponent[];
}


interface Schema {
    "$": {"xmlns:xs": string; elementFormDefault: string;};
    "xs:element": ModelComponent[];
    "xs:simpleType": TypeDefinition[];
}


interface TypeDefinition {
    "$": {name: string;};
    "xs:annotation": Annotation[];
    "xs:restriction": Restriction[];
}


interface Restriction {
    "xs:enumeration": Enumeration[];
}


interface Enumeration {
    "$": {value: string;};
}


const isParameter = (element: Element): element is Parameter => {
    return element["$"].hasOwnProperty("type");
}


const isPattern = (element: Element): element is Pattern => {
    return element["$"].hasOwnProperty("minOccurs") && element["$"].hasOwnProperty("maxOccurs");
}


export const fromXSD = (xsdContent: string): Model.Model => {
    var schema: Schema = {
        "$": {
            "xmlns:xs": "http://www.w3.org/2001/XMLSchema",
            elementFormDefault: "qualified",
        },
        "xs:element": [],
        "xs:simpleType": [],
    };

    parseXSD(xsdContent, function(err: Error, result: {"xs:schema": Schema}) {
        schema = result["xs:schema"];
    });

    const modelNode = schema["xs:element"][0];
    const infos = modelNode["xs:annotation"][0]["xs:documentation"][0];

    const rootNode = modelNode["xs:complexType"][0]["xs:sequence"][0]["xs:element"][0];
    const rootInfos = rootNode["xs:annotation"][0]["xs:documentation"][0];

    const model = new Model.Model(
        rootNode["$"].name.toString(),
        rootInfos.name.toString(),
        rootInfos.description.toString(),
        infos.version.toString());

    componentFromXSD(model.root, rootNode["xs:complexType"][0]["xs:sequence"][0]["xs:element"]);

    model.environment.addType("boolean", "Boolean", "Boolean primitive type.");
    model.environment.addType("byte", "Byte", "Byte primitive type.");
    model.environment.addType("char", "Char", "Char primitive type.");
    model.environment.addType("double", "Double", "Double primitive type.");
    model.environment.addType("float", "Float", "Float primitive type.");
    model.environment.addType("int", "Int", "Int primitive type.");
    model.environment.addType("long", "Long", "Long primitive type.");
    model.environment.addType("longdouble", "Long Double", "Long Double primitive type.");
    model.environment.addType("short", "Short", "Short primitive type.");
    model.environment.addType("string", "String", "String primitive type.");

    schema["xs:simpleType"].forEach((e: TypeDefinition) => {
        const infos = e["xs:annotation"][0]["xs:documentation"][0];
        const enumeration = model.environment.addEnum(
            e["$"].name.toString(),
            infos.name.toString(),
            infos.description.toString());

        e["xs:restriction"][0]["xs:enumeration"].forEach((v: Enumeration) => {
            enumeration.addValue(v["$"].value.toString());
        })
    })

    return model;
};


const componentFromXSD = (component: Model.Component, node: Element[]) => {
    if(node != null) {
        node.forEach((elmt: Parameter | Component) => {
            const id = elmt["$"].name;
            const infos = elmt["xs:annotation"][0]["xs:documentation"][0];

            if (isParameter(elmt)) {
                const parameter = component.addParameter(
                    infos.type.toString(),
                    id.toString(),
                    infos.name.toString(),
                    infos.description.toString(),
                    infos.unit.toString())

                parameter.refPath = infos.reference.toString()
                parameter.refValue = infos.expected.toString()
                parameter.advanced = infos.advanced.toString() === "true";

                /*
                const default = infos.default.toString();
                if (default !== "") {
                    parameter.value = default;
                }
                */
            } else {
                const element = elmt["xs:complexType"][0]["xs:sequence"][0]["xs:element"];
                if (infos.type.toString() === "component") {
                    const child = component.addChild(id.toString(), infos.name.toString(), infos.description.toString());
                    child.refPath = infos.reference.toString();
                    child.refValue = infos.expected.toString();
                    child.advanced = infos.advanced.toString() === "true";

                    componentFromXSD(child, element);
                } else if (infos.type.toString() === "list") {
                    const pattern = element[0];
                    if (!isPattern(pattern)) {
                        /* ??? Error ??? */
                        return;
                    }
                    const patternId = pattern["$"].name;
                    const minOccurences = Number(pattern["$"].minOccurs);
                    const minOccurs = isNaN(minOccurences) ? 0 : Math.max(minOccurences, 0);
                    const maxOccurences = Number(pattern["$"].maxOccurs);
                    const maxOccurs = isNaN(maxOccurences) ? Infinity : Math.max(minOccurs, maxOccurences);
                    const patternInfos = pattern["xs:annotation"][0]["xs:documentation"][0];
                    const child = component.addList(
                        id.toString(), infos.name.toString(), infos.description.toString(),
                        patternId.toString(), patternInfos.name.toString(), patternInfos.description.toString(),
                        minOccurs, maxOccurs);

                    child.refPath = infos.reference.toString();
                    child.refValue = infos.expected.toString();
                    child.advanced = infos.advanced.toString() === "true";

                    componentFromXSD(child.pattern, pattern["xs:complexType"][0]["xs:sequence"][0]["xs:element"]);
                } else {
                    /* ??? Error ??? */
                }
            }
        })
    }
};


/*

const builder = require('xmlbuilder');


export const toXML = (model: Model.Model) => {
    var configuration = builder.create('Configuration', { encoding: 'UTF-8' });
        configuration.att('version', model.version);
        configuration.att('id', model.id);
    var datamodel = configuration.ele('DataModel');

    componentToXML(model.root, datamodel);

    var xml = configuration.end({
        pretty: true,
        newline: '\n',
        allowEmpty: false
    });

    return xml;
}

let componentToXML = (component: Model.Component, parentNode: any) => {
    var componentNode = parentNode.ele(component.id);
    componentNode.att('MT', 'C');

    component.parameters.forEach(p => {
        componentNode.ele(p.id, {'MT' : 'P'}, p.value);
    });

    component.childs.forEach(c => {
        componentToXML(c, componentNode);
    });

    component.lists.forEach(l => {
        listToXML(l, componentNode);
    });
}

let listToXML = (list: Model.List, parentNode: any) => {
    var listNode = parentNode.ele(list.id);
    listNode.att('MT', 'L');

    list.elements.forEach(e => {
        componentToXML(e, listNode);
    });
}

export const fromXML = (model: Model.Model, xmlContent: string) => {
    var jsonModel;

    xml2js.parseString(xmlContent, function (err: any, result:any) {
        jsonModel = JSON.parse(JSON.stringify(result));
    });

    return schemaFromXML(model, jsonModel);
}

let schemaFromXML = (model: Model.Model, jsonObject: any) => {
    var schema = jsonObject["Configuration"]["DataModel"][0]["root"][0];

    componentFromXML(model.root, schema);

    return model;
}

let cloneLists = (parent: Model.List, pattern: Model.Component, clone: Model.Component) => {
    pattern.lists.forEach(l => {
        let cloneL = clone.addList(l.id, l.name, l.description, l.pattern.description);
        cloneL.refPath = l.refPath.replace('*', parent.elements.length.toString());
        cloneL.refValue = l.refValue;
        cloneL.visibility = l.visibility;
        cloneLists(parent, l.pattern, cloneL.pattern);
        cloneChilds(parent, l.pattern, cloneL.pattern);
        cloneParameters(parent, l.pattern, cloneL.pattern);
    })
}

let cloneChilds = (parent: Model.List, pattern: Model.Component, clone: Model.Component) => {
    pattern.childs.forEach(c => {
        let cloneC = clone.addChild(c.id, c.name, c.description);
        cloneC.refPath = c.refPath.replace('*', parent.elements.length.toString());
        cloneC.refValue = c.refValue;
        cloneC.visibility = c.visibility;
        cloneLists(parent, c, cloneC);
        cloneChilds(parent, c, cloneC);
        cloneParameters(parent, c, cloneC);
    })
}

let cloneParameters = (parent: Model.List, pattern: Model.Component, clone: Model.Component) => {
    pattern.parameters.forEach(p => {
        let cloneP = clone.addParameter(p.type, p.id, p.name, p.description, p.unit);
        cloneP.value = p.value;
        cloneP.refPath = p.refPath.replace('*', parent.elements.length.toString());
        cloneP.refValue = p.refValue;
        cloneP.visibility = p.visibility;
    });
}

let addItem = (list: Model.List) => {
    let patternClone = new Model.Component(list.id + "_item", "Item " + list.elements.length, list.pattern.description);
    patternClone.model = list.model;

    cloneLists(list, list.pattern, patternClone);
    cloneChilds(list, list.pattern, patternClone);
    cloneParameters(list, list.pattern, patternClone);

    list.elements.push(patternClone);

    return list.elements[list.elements.length - 1];
}

let componentFromXML = (component: Model.Component, jsonObject: any) => {
    Object.entries(jsonObject).slice(1).forEach((node: any) => {
        var nodeId = node[0].toString();
        var node = node[1][0];

        console.log(nodeId);
        console.log(node);

        switch(node["$"]["MT"].toString()) {
            case 'P':
                var p = component.parameters.find(p => {
                    return p.id === nodeId;
                });

                if(p && node["_"]) {
                    p.value = node["_"].toString();
                }
            break;
            case 'C':
                var c = component.childs.find(c => {
                    return c.id === nodeId;
                });

                if(c) {
                    componentFromXML(c, node);
                }
            break;
            case 'L':
                var l = component.lists.find(l => {
                    return l.id === nodeId;
                });

                if(l) {
                    var list = l;
                    if(node[l.id + "_item"]) {
                        var i = 0;

                        Object.entries(node[l.id + "_item"]).forEach((e: any) => {
                            var element = (list.elements[i]) ? list.elements[i] : addItem(list);
                            var elementNode = e[1];
                            componentFromXML(element, elementNode);

                            i += 1;
                        });
                    }
                }
            break;
        }
    })
}
*/
