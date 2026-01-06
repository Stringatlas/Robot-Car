#ifndef CLIENTCONTROLMANAGER_H
#define CLIENTCONTROLMANAGER_H

#include <Arduino.h>
#include <functional>

class ClientControlManager {
public:
    using ControlStatusCallback = std::function<void(uint32_t controllingClientId)>;

    ClientControlManager();
    
    void onControlStatusChanged(ControlStatusCallback callback);
    
    bool requestControl(uint32_t clientId);
    bool releaseControl(uint32_t clientId);
    void handleClientDisconnect(uint32_t clientId);
    
    bool hasControl(uint32_t clientId) const;
    uint32_t getControllingClientId() const { return controllingClientId; }
    
    void grantControlToFirstClient(uint32_t clientId);

private:
    uint32_t controllingClientId;
    ControlStatusCallback statusCallback;
    
    void notifyControlStatusChanged();
};

#endif
