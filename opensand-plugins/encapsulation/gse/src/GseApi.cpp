#include "GseApi.h"

#include <cstring>

bool decapstatus_to_string(RustDecapStatusType status, char *string)
{
    switch (status)
    {
    case RustDecapStatusType::DecapCompletedPkt:
        strcpy(string, "DecapCompletedPkt");
        break;
    case RustDecapStatusType::DecapFragmentedPkt:
        strcpy(string, "DecapFragmentedPkt");
        break;
    case RustDecapStatusType::DecapPadding:
        strcpy(string, "DecapPadding");
        break;
    case RustDecapStatusType::DecapErrorSizeBuffer:
        strcpy(string, "DecapErrorSizeBuffer");
        break;
    case RustDecapStatusType::DecapErrorTotalLength:
        strcpy(string, "DecapErrorTotalLength");
        break;
    case RustDecapStatusType::DecapErrorGseLength:
        strcpy(string, "DecapErrorGseLength");
        break;
    case RustDecapStatusType::DecapErrorSizePduBuffer:
        strcpy(string, "DecapErrorSizePduBuffer");
        break;
    case RustDecapStatusType::DecapErrorMemoryMemoryCorrupted:
        strcpy(string, "DecapErrorMemoryMemoryCorrupted");
        break;
    case RustDecapStatusType::DecapErrorMemoryBufferTooSmall:
        strcpy(string, "DecapErrorMemoryBufferTooSmall");
        break;
    case RustDecapStatusType::DecapErrorMemoryStorageOverflow:
        strcpy(string, "DecapErrorMemoryStorageOverflow");
        break;
    case RustDecapStatusType::DecapErrorMemoryStorageUnderflow:
        strcpy(string, "DecapErrorMemoryStorageUnderflow");
        break;
    case RustDecapStatusType::DecapErrorMemoryUndefinedId:
        strcpy(string, "DecapErrorMemoryUndefinedId");
        break;
    case RustDecapStatusType::DecapErrorCRC:
        strcpy(string, "DecapErrorCRC");
        break;
    case RustDecapStatusType::DecapErrorProtocolType:
        strcpy(string, "DecapErrorProtocolType");
        break;
    case RustDecapStatusType::DecapErrorInvalidLabel:
        strcpy(string, "DecapErrorInvalidLabel");
        break;
    default:
        return false; // Unknown enum value
    }
    return true;
}

bool encapstatus_to_string(RustEncapStatusType status, char *string)
{
    switch (status)
    {
    case RustEncapStatusType::EncapCompletedPkt:
        strcpy(string, "EncapCompletedPkt");
        break;
    case RustEncapStatusType::EncapFragmentedPkt:
        strcpy(string, "EncapFragmentedPkt");
        break;
    case RustEncapStatusType::EncapErrorPduLength:
        strcpy(string, "EncapErrorPduLength");
        break;
    case RustEncapStatusType::EncapErrorSizeBuffer:
        strcpy(string, "EncapErrorSizeBuffer");
        break;
    case RustEncapStatusType::EncapErrorSizePduBuffer:
        strcpy(string, "EncapErrorSizePduBuffer");
        break;
    case RustEncapStatusType::EncapErrorProtocolType:
        strcpy(string, "EncapErrorProtocolType");
        break;
    case RustEncapStatusType::EncapErrorInvalidLabel:
        strcpy(string, "EncapErrorInvalidLabel");
        break;
    default:
        return false; // Unknown enum value
    }
    return true;
}
