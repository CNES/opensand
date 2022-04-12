import {createSlice, PayloadAction} from '@reduxjs/toolkit';


const SSH_CONFIGURATION_TIMEOUT = 20;  // minutes


declare type ActionCallback = () => void;


interface SshState {
    password: string;
    isPassphrase: boolean;
    show: boolean;
    open: boolean;
    configured: boolean;
    lastAction: number;
    nextAction?: ActionCallback;
}


const initialState: SshState = {
    password: "",
    isPassphrase: false,
    show: false,
    open: false,
    configured: false,
    lastAction: Number(new Date()),
};


const sshSlice = createSlice({
    name: "ssh",
    initialState,
    reducers: {
        changePasswordVisibility: (state) => {
            return {
                ...state,
                show: !state.show,
            };
        },
        configureSsh: (state, action: PayloadAction<{password: string; isPassphrase: boolean;}>) => {
            return {
                ...state,
                ...action.payload,
                lastAction: Number(new Date()),
                configured: true,
            };
        },
        runSshCommand: (state, action: PayloadAction<{action: ActionCallback;}>) => {
            const nextAction = action.payload.action;
            const now = new Date();
            const expired = new Date(state.lastAction);
            expired.setMinutes(expired.getMinutes() + SSH_CONFIGURATION_TIMEOUT);

            if (state.configured && now < expired) {
                nextAction();
                return {
                    ...state,
                    lastAction: Number(now),
                };
            }

            return {
                ...state,
                open: true,
                nextAction,
            };
        },
        openSshDialog: (state) => {
            return {
                ...state,
                open: true,
            };
        },
        closeSshDialog: (state) => {
            return {
                ...state,
                open: false,
                nextAction: undefined,
            };
        },
    },
});


export const {changePasswordVisibility, configureSsh, runSshCommand, openSshDialog, closeSshDialog} = sshSlice.actions;
export default sshSlice.reducer;
