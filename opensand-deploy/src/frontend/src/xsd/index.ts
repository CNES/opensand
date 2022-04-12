export type {Model, Element, Parameter, Component, List, Visibility} from './model';
export {
    isParameterElement,
    isComponentElement,
    isListElement,
    isVisible,
    isComplete,
    newItem,
    isReadOnly,
    getParameters,
    getComponents,
    clone,
    Visibilities,
} from './model';
export {toXML} from './serialiser';
export {fromXSD, fromXML} from './parser';


export const getXsdName = (parameter_id: string, entity_type?: string) => {
    if (parameter_id !== "profile") {
        return parameter_id + ".xsd";
    }

    switch (entity_type) {
        case "Gateway":
            return "profile_gw.xsd";
        case "Gateway Net Access":
            return "profile_gw_net_acc.xsd";
        case "Gateway Phy":
            return "profile_gw_phy.xsd";
        case "Terminal":
            return "profile_st.xsd";
        default:
            return "";
/*
        case "Satellite":
            return "profile_sat.xsd";
        default:
            return "profile.xsd";
*/
    }
};
