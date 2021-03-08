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
    model: Model;

    constructor(id: string, name: string, description: string, model: Model) {
        super(id, name, description);
        this.model = model;
        this.refPath = "";
        this.refValue = "";
        this.advanced = false;
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
        this.saved = false;
    }

    isComplete = (): boolean => {
        return this.root.isValid();
    };

    getPathValue = (path: string): string => {
        const splitPath = path.split("/").slice(1);
        const [id, ...subPath] = splitPath;

        const component = this.root.children.find((c: Component) => c.id === id);
        if (component) return component.getPathValueInternal(subPath);

        const list = this.root.lists.find((l: List) => l.id === id);
        if (list) return list.getPathValueInternal(subPath);

        const parameter = this.root.parameters.find((p: Parameter) => p.id === id);
        if (parameter) return parameter.getPathValueInternal(subPath);

        return "";
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
    children: Component[] = [];
    parameters: Parameter[] = [];
    lists: List[] = [];

    isVisible(): boolean {
        if (!super.isVisible()) {
            return false;
        }

        const count =
            this.children.filter((c: Component) => c.isVisible()).length +
            this.lists.filter((l: List) => l.isVisible()).length +
            this.parameters.filter((p: Parameter) => p.isVisible()).length;
        return count > 0;
    }

    addParameter = (type: string, id: string, name: string, description: string, unit: string): Parameter => {
        const parameter = new Parameter(type, id, name, description, unit, this.model);
        this.parameters.push(parameter);
        return parameter;
    };

    addChild = (id: string, name: string, description: string): Component => {
        const child = new Component(id, name, description, this.model);
        this.children.push(child);
        return child;
    };

    addList = (id: string, name: string, description: string, patternId: string, patternName: string, patternDescription: string, minOccurences: number, maxOccurences: number): List => {
        const list = new List(id, name, description, patternId, patternName, patternDescription, minOccurences, maxOccurences, this.model);
        this.lists.push(list);
        return list;
    };

    isValid = (): boolean => {
        let valid = true;

        this.parameters.forEach((p: Parameter) => {
            const v = p.value !== "" || !p.isVisible();
            if(!v){ console.log(p.name) }
            valid = valid && v;
        })

        this.children.forEach((c: Component) => {
            const v = c.isValid() || !c.isVisible();
            if(!v){ console.log(c.name) }
            valid = valid && v;
        })

        this.lists.forEach((l: List) => {
            l.elements.forEach((e: Component) => {
                const v = e.isValid() || !e.isVisible() || !l.isVisible();
                if(!v){ console.log(e.name) }
                valid = valid && v;
            })
        })

        return valid;
    };

    getPathValueInternal = (path: string[]): string => {
        const [id, ...subPath] = path;

        const component = this.children.find((c: Component) => c.id === id);
        if (component) return component.getPathValueInternal(subPath);

        const list = this.lists.find((l: List) => l.id === id);
        if (list) return list.getPathValueInternal(subPath);

        const parameter = this.parameters.find((p: Parameter) => p.id === id);
        if (parameter) return parameter.getPathValueInternal(subPath);

        return "";
    };

    clone = (parent: Component, index: number) => {
        parent.lists.forEach((l: List) => {
            const cloned = this.addList(l.id, l.name, l.description, l.pattern.id, l.pattern.name, l.pattern.description, l.minOccurences, l.maxOccurences);
            cloned.refPath = l.refPath.replace('*', index.toString());
            cloned.refValue = l.refValue;
            cloned.advanced = l.advanced;
            cloned.pattern.refPath = l.pattern.refPath.replace('*', index.toString());
            cloned.pattern.refValue = l.pattern.refValue;
            cloned.pattern.advanced = l.pattern.advanced;
            cloned.pattern.clone(l.pattern, index);
        });
        parent.children.forEach((c: Component) => {
            const cloned = this.addChild(c.id, c.name, c.description);
            cloned.refPath = c.refPath.replace('*', index.toString());
            cloned.refValue = c.refValue;
            cloned.advanced = c.advanced;
            cloned.clone(c, index);
        });
        parent.parameters.forEach((p: Parameter) => {
            const cloned = this.addParameter(p.type, p.id, p.name, p.description, p.unit);
            cloned.refPath = p.refPath.replace('*', index.toString());
            cloned.refValue = p.refValue;
            cloned.advanced = p.advanced;
            cloned.value = p.value;
        });
    };
}


export class List extends NamedElement {
    minOccurences: number;
    maxOccurences: number;

    pattern: Component;
    elements: Component[] = [];
    selectedElement: Component | null = null;

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
