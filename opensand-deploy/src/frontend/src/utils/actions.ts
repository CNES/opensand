import type {Component, List} from '../xsd';


interface IEntity {
    readonly name: string;
    readonly type: string;
}


export type MutatorCallback = (list: List, path: string, creator: (l: List) => Component | undefined) => void;
export interface IAction {
    readonly onEdit?: (entity: IEntity | undefined, key: string, xsd: string, xml?: string) => void;
    readonly onRemove?: (entity: string | undefined, key: string) => void;
    readonly onCreate?: (root: Component, mutate: MutatorCallback, submitForm: () => void) => void;
    readonly onDelete?: (root: Component, mutate: (list: List, path: string) => void) => void;
    readonly onAction?: (index: number) => JSX.Element;
}


export interface IActions {
    readonly '$': IAction;
    readonly '#': {[id: string]: IActions;};
}


export const noActions: IActions = {
  '$': {},
  '#': {},
};


interface IActionSpec {
    readonly path: string[];
    readonly actions: IAction;
};


const mergeActions = (first?: IActions, second?: IActions): IActions => {
    if (first == null) { return second ? second : noActions; }
    if (second == null) { return first; }

    const keys = new Map<string, boolean>();
    Object.keys(first['#']).forEach(s => keys.set(s, true));
    Object.keys(second['#']).forEach(s => keys.set(s, true));

    const result: {[id: string]: IActions;} = {};
    keys.forEach((value: boolean, key: string) => { result[key] = mergeActions(first['#'][key], second['#'][key]); });

    return {
        '$': {...first.$, ...second.$},
        '#': result,
    };
};


export const combineActions = (actions: IActionSpec[]): IActions => {
    return actions.map((spec: IActionSpec) => {
        let action: IActions = {'$': spec.actions, '#': {}};
        for (let i = spec.path.length; i > 0; --i) {
            const name = spec.path[i - 1];
            action = {'$': {}, '#': {[name]: action}};
        }
        return action;
    }).reduce((accumulator: IActions, current: IActions) => {
        return mergeActions(accumulator, current);
    });
};
