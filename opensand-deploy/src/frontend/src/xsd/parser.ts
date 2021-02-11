import * as Model from './model';

import {parseString as parseXML} from 'xml2js';


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
    "xs:simpleType"?: TypeDefinition[];
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

    parseXML(xsdContent, function(err: Error, result: {"xs:schema": Schema}) {
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

    schema["xs:simpleType"]?.forEach((e: TypeDefinition) => {
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

                    child.pattern.refPath = patternInfos.reference.toString();
                    child.pattern.refValue = patternInfos.expected.toString();
                    child.pattern.advanced = patternInfos.advanced.toString() === "true";

                    componentFromXSD(child.pattern, pattern["xs:complexType"][0]["xs:sequence"][0]["xs:element"]);
                } else {
                    /* ??? Error ??? */
                }
            }
        })
    }
};


interface XMLElement {
    [name: string]: XMLElement[] | string[];
}


interface DataModel {
    "$": {version: string;};
    root: XMLElement[];
}


const isXmlElement = (element: XMLElement | string): element is XMLElement => {
    return typeof(element) !== "string";
};


export const fromXML = (model: Model.Model, xmlContent: string) => {
    var dataModel: DataModel = {
        "$": {version: model.version},
        root: [],
    };

    parseXML(xmlContent, function (err: Error, result: {model: DataModel;}) {
        dataModel = result.model;
    });

    if (model.version === dataModel["$"].version) {
        fillComponentFromXML(model.root, dataModel.root[0]);
    }

    return model;
};


const fillComponentFromXML = (component?: Model.Component, xml?: XMLElement) => {
    if (component == null || xml == null) {
        return;
    }

    component.children.forEach((c: Model.Component) => {
        if (xml.hasOwnProperty(c.id)) {
            const element = xml[c.id][0];
            if (isXmlElement(element)) {
                fillComponentFromXML(c, element);
            }
        }
    });

    component.lists.forEach((l: Model.List) => {
        const patternId = l.pattern.id + "_item_";
        if (xml.hasOwnProperty(l.id)) {
            const xmlElements = xml[l.id][0];
            if (!isXmlElement(xmlElements)) {
                // Pretty unlikely, but we shall follow typescript instructions
                return;
            }

            const keys = Object.keys(xmlElements).filter((key: string) => key.startsWith(patternId));
            keys.forEach((key: string) => {
                const id = Number(key.substr(patternId.length));
                if (isNaN(id)) {
                    return;
                }

                const element = xmlElements[key][0];
                if (!isXmlElement(element)) {
                    return;
                }

                let length = l.elements.length;
                while (id >= length) {
                    l.addItem();
                    if (l.elements.length === length) {
                        // reached maxOccurences
                        return;
                    }
                    length = l.elements.length;
                }

                fillComponentFromXML(l.elements[id], element);
            });
        }
    });

    component.parameters.forEach((p: Model.Parameter) => {
        if (xml.hasOwnProperty(p.id)) {
            const element = xml[p.id][0];
            if (!isXmlElement(element)) {
                p.value = element;
            }
        }
    });
};
