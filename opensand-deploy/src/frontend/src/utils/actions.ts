import type {Component, List} from '../xsd';


interface IEntity {
    readonly name: string;
    readonly type: string;
}


export type MutatorCallback = (list: List, path: string, creator: (l: List) => Component | undefined) => void;


export interface IXsdAction {
    readonly onEdit: (entity: IEntity | undefined, key: string, xsd: string, xml?: string) => void;
    readonly onRemove: (entity: string | undefined, key: string) => void;
}


export interface IProjectAction {
    readonly onCreate: (mutate: MutatorCallback, submitForm: () => void) => void;
    readonly onDelete: (mutate: (list: List, path: string) => void) => void;
}
