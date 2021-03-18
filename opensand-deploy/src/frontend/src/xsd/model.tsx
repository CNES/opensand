export const Visibilities = ["FULL", "ADVANCED", "NORMAL"] as const;
export type Visibility = typeof Visibilities[number];


export class Named {
    id: string;
    name: string;
    description: string;

    constructor(id: string, name: string, description: string) {
        this.id = id;
        this.name = name;
        this.description = description;
    }
}


export class NamedElement extends Named {
    refPath: string;
    refValue: string;
    advanced: boolean;
    readOnly: boolean;
    model: Model;

    constructor(id: string, name: string, description: string, model: Model) {
        super(id, name, description);
        this.model = model;
        this.refPath = "";
        this.refValue = "";
        this.advanced = false;
        this.readOnly = false;
    }

    isVisible(): boolean {
        if(this.model.visibility === "FULL") {
            return true;
        }

        const drawable = this.refPath === "" ? true : this.model.getPathValue(this.refPath) === this.refValue;
        return drawable && (!this.advanced || this.model.visibility !== "NORMAL");
    }
}


export class Model {
    version: string;
    environment: Environment;
    root: Component;
    visibility: Visibility;
    saved: boolean;

    constructor(id: string, name: string, description: string, version: string) {
        this.version = version;
        this.visibility = "NORMAL";
        this.environment = new Environment();
        this.root = new Component(id, name, description === "" ? "Model" : description, this);
        this.saved = true;
    }

    isComplete = (): boolean => {
        return this.root.isValid();
    };

    getPathValue = (path: string): string => {
        const splitPath = path.split("/").slice(1);
        return this.root.getPathValueInternal(splitPath);
    };
}


export class Environment {
    types: Named[] = [];
    enums: Enum[] = [];

    addType = (id: string, name: string, description: string): Named => {
        const type = new Named(id, name, description);
        this.types.push(type);
        return type;
    };

    addEnum = (id: string, name: string, description: string): Enum => {
        const enumeration = new Enum(id, name, description);
        this.enums.push(enumeration);
        return enumeration;
    };
}


export class Enum extends Named {
    values: string[] = [];

    addValue = (value: string) => {
        this.values.push(value);
    };
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


export class Parameter extends NamedElement {
    type: string;
    unit: string;
    value: string;

    constructor(type: string, id: string, name: string, description: string, unit: string, model: Model) {
        super(id, name, description, model);
        this.type = type;
        this.unit = unit;
        this.value = type === "boolean" ? "false" : "";
    }

    getPathValueInternal = (path: string[]): string => {
        return path.length === 0 ? this.value : "";
    };
}


export class Component extends NamedElement {
    elements: Element[] = [];

    isVisible(): boolean {
        if (!super.isVisible()) {
            return false;
        }

        const count = this.elements.filter((e: Element) => e.element.isVisible()).length;
        return count > 0;
    }

    addParameter = (type: string, id: string, name: string, description: string, unit: string): Parameter => {
        const parameter = new Parameter(type, id, name, description, unit, this.model);
        const element: ParameterElement = {type: "parameter", element: parameter};
        this.elements.push(element);
        return parameter;
    };

    addChild = (id: string, name: string, description: string): Component => {
        const child = new Component(id, name, description, this.model);
        const element: ComponentElement = {type: "component", element: child};
        this.elements.push(element);
        return child;
    };

    addList = (id: string, name: string, description: string, patternId: string, patternName: string, patternDescription: string, minOccurences: number, maxOccurences: number): List => {
        const list = new List(id, name, description, patternId, patternName, patternDescription, minOccurences, maxOccurences, this.model);
        const element: ListElement = {type: "list", element: list};
        this.elements.push(element);
        return list;
    };

    getComponents = (checkVisibility: boolean = true): Component[] => {
        const keepComponents = ((e: Element): e is ComponentElement => e.type === "component" && (!checkVisibility || e.element.isVisible()));
        const components: ComponentElement[] = this.elements.filter(keepComponents);
        return components.map((e: ComponentElement) => e.element);
    };

    getParameters = (checkVisibility: boolean = true): Parameter[] => {
        const keepParameters = ((e: Element): e is ParameterElement => e.type === "parameter" && (!checkVisibility || e.element.isVisible()));
        const parameters: ParameterElement[] = this.elements.filter(keepParameters);
        return parameters.map((e: ParameterElement) => e.element);
    };

    isValid = (): boolean => {
        return this.elements.reduce((valid: boolean, element: Element) => {
            switch (element.type) {
                case "parameter":
                    const parameter: Parameter = element.element;
                    if (parameter.value === "" && parameter.isVisible()) {
                        console.log(parameter.name);
                        return false;
                    }
                    return valid;
                case "component":
                    const component: Component = element.element;
                    if (!component.isValid() && component.isVisible()) {
                        console.log(component.name);
                        return false;
                    }
                    return valid;
                case "list":
                    const list: List = element.element;
                    return list.elements.reduce((v: boolean, e: Component) => {
                        if (!e.isValid() && e.isVisible() && list.isVisible()) {
                            console.log(e.name);
                            return false;
                        }
                        return v;
                    }, valid);
                default:
                    console.log("Unknown component type", element);
                    return false;
            }
        }, true);
    };

    getPathValueInternal = (path: string[]): string => {
        const [id, ...subPath] = path;

        const element = this.elements.find((e: Element) => e.element.id === id);
        if (element) {
            return element.element.getPathValueInternal(subPath);
        }

        return "";
    };

    clone = (parent: Component, index: number) => {
        parent.elements.forEach((e: Element) => {
            switch (e.type) {
                case "list": {
                    const cloned = this.addList(
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
                    cloned.pattern.clone(e.element.pattern, index);
                    break;
                }
                case "component": {
                    const cloned = this.addChild(e.element.id, e.element.name, e.element.description);
                    cloned.refPath = e.element.refPath.replace('*', index.toString());
                    cloned.refValue = e.element.refValue;
                    cloned.advanced = e.element.advanced;
                    cloned.readOnly = e.element.readOnly;
                    cloned.clone(e.element, index);
                    break;
                }
                case "parameter": {
                    const cloned = this.addParameter(
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
}


export class List extends NamedElement {
    minOccurences: number;
    maxOccurences: number;

    pattern: Component;
    elements: Component[] = [];

    constructor(id: string, name: string, description: string,
                patternId: string, patternName: string, patternDescription: string,
                minOccurences: number, maxOccurences: number, model: Model) {
        super(id, name, description, model);
        this.minOccurences = minOccurences;
        this.maxOccurences = maxOccurences;
        this.pattern = new Component(patternId, patternName, patternDescription, model);

        // Array.from(Array(minOccurences)).forEach(this.addItem);
        for (let i = 0; i < minOccurences; i++) {
            this.addItem();
        }
    }

    isVisible(): boolean {
        if (!super.isVisible()) {
            return false;
        }

        return this.pattern.isVisible();
    }

    isReadOnly = (): boolean => {
        return this.readOnly || this.pattern.readOnly;
    };

    getPathValueInternal = (path: string[]): string => {
        const [index, ...subPath] = path;

        const element = this.elements[Number(index)];
        if (element) return element.getPathValueInternal(subPath);

        return "";
    };

    addItem = () => {
        if (this.elements.length < this.maxOccurences) {
            const {name, description} = this.pattern;
            const index = this.elements.length;
            const item = new Component("item", name + " " + index, description, this.model);
            item.refPath = this.pattern.refPath.replace('*', index.toString());
            item.refValue = this.pattern.refValue;
            item.advanced = this.pattern.advanced;
            item.readOnly = this.pattern.readOnly;
            item.clone(this.pattern, index);

            this.elements.push(item);
        }
    };

    removeItem = () => {
        if (this.elements.length > this.minOccurences) {
            this.elements.pop();
        }
    };
}
