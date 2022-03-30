export const Visibilities = ["FULL", "ADVANCED", "NORMAL"] as const;
export type Visibility = typeof Visibilities[number];


interface Named {
    id: string;
    name: string;
    description: string;
}


interface NamedElement extends Named {
    refPath: string;
    refValue: string;
    advanced: boolean;
    readOnly: boolean;
}


export interface Model {
    version: string;
    environment: Environment;
    root: Component;
    saved: boolean;
}

interface Enum extends Named {
    values: string[];
}


interface Environment {
    types: Named[];
    enums: Enum[];
}


interface ParameterElement {
    type: "parameter";
    element: Parameter;
}


interface ComponentElement {
    type: "component";
    element: Component;
}


interface ListElement {
    type: "list";
    element: List;
}


export type Element = ParameterElement | ComponentElement | ListElement;


export interface Parameter extends NamedElement {
    type: string;
    unit: string;
    value: string;
}


export interface Component extends NamedElement {
    elements: Element[];
}


export interface List extends NamedElement {
    minOccurences: number;
    maxOccurences: number;
    pattern: Component;
    elements: Component[];
}


const isContainer = (element: NamedElement): element is Component | List => {
    return element.hasOwnProperty("elements");
};


const isList = (element: Component | List): element is List => {
    return element.hasOwnProperty("pattern");
};


export const isParameterElement = (e?: Element): e is ParameterElement => e != null && e.type === "parameter";
export const isComponentElement = (e?: Element): e is ComponentElement => e != null && e.type === "component";
export const isListElement = (e?: Element): e is ListElement => e != null && e.type === "list";


export const newNamedElement = (id: string, name: string, description: string): NamedElement => {
    return {
        id, name, description,
        refPath: "",
        refValue: "",
        advanced: false,
        readOnly: false,
    };
};


export const newParameter = (id: string, name: string, description: string, type: string, unit: string): Parameter => {
    const value = type === "boolean" ? "false" : "";
    return {
        ...newNamedElement(id, name, description),
        type, unit, value,
    };
};


export const newComponent = (id: string, name: string, description: string): Component => {
    return {
        ...newNamedElement(id, name, description),
        elements: [],
    };
};


export const newList = (id: string, name: string, description: string, pattern: Component, minOccurences: number, maxOccurences: number): List => {
    const list: List = {
        ...newNamedElement(id, name, description),
        minOccurences,
        maxOccurences,
        pattern,
        elements: [],
    };

    for (let i = 0; i < minOccurences; ++i) {
        addItem(list);
    }
    return list;
};


export const isVisible = (element: NamedElement, visibility: Visibility, root: Component): boolean => {
    const {advanced, refPath, refValue} = element;
    if (visibility === "FULL") {
        return true;
    }

    const drawable = refPath === "" ? true : getPathValueComponent(root, refPath.split("/").slice(1)) === refValue;
    const visible = drawable && (!advanced || visibility !== "NORMAL");

    if (visible && isContainer(element)) {
        if (isList(element)) {
            return isVisible(element.pattern, visibility, root);
        } else {
            return element.elements.filter((e: Element) => isVisible(e.element, visibility, root)).length > 0;
        }
    }

    return visible;
};


export const isComplete = (model: Model, visibility: Visibility) => {
    return isValid(model.root, model.root, visibility);
};


const getPathValueComponent = (component: Component, path: string[]): string => {
    const [id, ...subPath] = path;

    const element = component.elements.find((e: Element) => e.element.id === id);
    if (isComponentElement(element)) {
        return getPathValueComponent(element.element, subPath);
    } else if (isListElement(element)) {
        return getPathValueList(element.element, subPath);
    } else if (isParameterElement(element)) {
        return getPathValueParameter(element.element, subPath);
    }

    return "";
};


const getPathValueList = (list: List, path: string[]): string => {
    const [index, ...subPath] = path;

    const element = list.elements[Number(index)];
    if (element) {
        return getPathValueComponent(element, subPath);
    }

    return "";
};


const getPathValueParameter = (parameter: Parameter, path: string[]): string => {
    if (path.length) {
        return "";
    }
    return parameter.value;
};


export const addType = (environment: Environment, id: string, name: string, description: string) => {
    const newType: Named = {id, name, description};
    environment.types.push(newType);
    return newType;
};


export const addEnum = (environment: Environment, id: string, name: string, description: string) => {
    const newEnum: Enum = {id, name, description, values: []};
    environment.enums.push(newEnum);
    return newEnum;
};


export const addValue = (enumeration: Enum, value: string) => {
    enumeration.values.push(value);
};


export const addParameter = (component: Component, type: string, id: string, name: string, description: string, unit: string) => {
    const parameter = newParameter(id, name, description, type, unit);
    component.elements.push({type: "parameter", element: parameter});
    return parameter;
};


export const addChild = (component: Component, id: string, name: string, description: string) => {
    const child = newComponent(id, name, description);
    component.elements.push({type: "component", element: child});
    return child;
};


export const addList = (component: Component, id: string, name: string, description: string, patternId: string, patternName: string, patternDescription: string, minOccurences: number, maxOccurences: number) => {
    const pattern = newComponent(patternId, patternName, patternDescription);
    const list = newList(id, name, description, pattern, minOccurences, maxOccurences);
    component.elements.push({type: "list", element: list});
    return list;
};


export const newItem = (pattern: Component, index: number) => {
    const {name, description, refPath, refValue, advanced, readOnly} = pattern;
    const item = newComponent("item", name + " " + index, description);
    item.refPath = refPath.replace('*', index.toString());
    item.refValue = refValue;
    item.readOnly = readOnly;
    item.advanced = advanced;
    clone(item, pattern, index);
    return item;
};


export const addItem = (list: List) => {
    const index = list.elements.length;
    if (index < list.maxOccurences) {
        const item = newItem(list.pattern, index);
        list.elements.push(item);
        return item;
    }
}; 


export const isReadOnly = (list: List) => {
    return list.readOnly || list.pattern.readOnly;
};


export const clone = (component: Component, parent: Component, index: number) => {
    parent.elements.forEach((e: Element) => {
        switch (e.type) {
            case "list": {
                const cloned = addList(
                    component,
                    e.element.id,
                    e.element.name,
                    e.element.description,
                    e.element.pattern.id,
                    e.element.pattern.name,
                    e.element.pattern.description,
                    e.element.minOccurences,
                    e.element.maxOccurences);
                cloned.refPath = e.element.refPath.replace('*', index.toString());
                cloned.refValue = e.element.refValue;
                cloned.advanced = e.element.advanced;
                cloned.readOnly = e.element.readOnly;
                cloned.pattern.refPath = e.element.pattern.refPath.replace('*', index.toString());
                cloned.pattern.refValue = e.element.pattern.refValue;
                cloned.pattern.advanced = e.element.pattern.advanced;
                cloned.pattern.readOnly = e.element.pattern.readOnly;
                clone(cloned.pattern, e.element.pattern, index);
                break;
            }
            case "component": {
                const cloned = addChild(component, e.element.id, e.element.name, e.element.description);
                cloned.refPath = e.element.refPath.replace('*', index.toString());
                cloned.refValue = e.element.refValue;
                cloned.advanced = e.element.advanced;
                cloned.readOnly = e.element.readOnly;
                clone(cloned, e.element, index);
                break;
            }
            case "parameter": {
                const cloned = addParameter(
                    component,
                    e.element.type,
                    e.element.id,
                    e.element.name,
                    e.element.description,
                    e.element.unit);
                cloned.refPath = e.element.refPath.replace('*', index.toString());
                cloned.refValue = e.element.refValue;
                cloned.advanced = e.element.advanced;
                cloned.readOnly = e.element.readOnly;
                cloned.value = e.element.value;
                break;
            }
        }
    });
};


const isValid = (component: Component, root: Component, visibility: Visibility): boolean => {
    return component.elements.reduce((valid: boolean, element: Element) => {
        if (valid) {
            if (!isVisible(element.element, visibility, root)) {
                return true;
            }
            switch (element.type) {
                case "parameter":
                    return element.element.value !== "";
                case "component":
                    return isValid(element.element, root, visibility);
                case "list":
                    return element.element.elements.reduce((v: boolean, c: Component) => {
                        return v && (isValid(c, root, visibility) || !isVisible(c, visibility, root));
                    }, true);
                default:
                    throw new Error("Unknown component type");
            }
        }
        return valid;
    }, true);
};


export const getParameters = (component: Component, root: Component, visibility?: Visibility) => {
    const components = component.elements.filter((e: Element): e is ParameterElement => (
        isParameterElement(e) && (!visibility || isVisible(e.element, visibility, root))
    ));
    return components.map((e: ParameterElement): Parameter => e.element);
};


export const getComponents = (component: Component, root: Component, visibility?: Visibility) => {
    const components = component.elements.map((e: Element, i: number): [number, Element] => (
        [i, e]
    )).filter((entry: [number, Element]): entry is [number, ComponentElement] => (
        isComponentElement(entry[1]) && (!visibility || isVisible(entry[1].element, visibility, root))
    ));
    return components.map(([i, e]: [number, ComponentElement]): [number, Component] => [i, e.element]);
};
