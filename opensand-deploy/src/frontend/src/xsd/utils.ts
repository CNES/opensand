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
        case "Satellite":
            return "profile_sat.xsd";
        default:
            return "profile.xsd";
    }
};
