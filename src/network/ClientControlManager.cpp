#include "ClientControlManager.h"
#include "Telemetry.h"

ClientControlManager::ClientControlManager() 
    : controllingClientId(0), statusCallback(nullptr) {}

void ClientControlManager::onControlStatusChanged(ControlStatusCallback callback) {
    statusCallback = callback;
}

bool ClientControlManager::requestControl(uint32_t clientId) {
    controllingClientId = clientId;
    TELEM_LOGF_INFO("Control granted to client #%u", clientId);
    notifyControlStatusChanged();
    return true;
}

bool ClientControlManager::releaseControl(uint32_t clientId) {
    if (controllingClientId == clientId) {
        controllingClientId = 0;
        TELEM_LOGF_INFO("Control released by client #%u", clientId);
        notifyControlStatusChanged();
        return true;
    }
    return false;
}

void ClientControlManager::handleClientDisconnect(uint32_t clientId) {
    if (controllingClientId == clientId) {
        controllingClientId = 0;
        TELEM_LOG_INFO("Control released (client disconnected)");
        notifyControlStatusChanged();
    }
}

bool ClientControlManager::hasControl(uint32_t clientId) const {
    return controllingClientId == clientId;
}

void ClientControlManager::grantControlToFirstClient(uint32_t clientId) {
    if (controllingClientId == 0) {
        controllingClientId = clientId;
        TELEM_LOGF_INFO("Client #%u automatically granted control", clientId);
        notifyControlStatusChanged();
    }
}

void ClientControlManager::notifyControlStatusChanged() {
    if (statusCallback) {
        statusCallback(controllingClientId);
    }
}
