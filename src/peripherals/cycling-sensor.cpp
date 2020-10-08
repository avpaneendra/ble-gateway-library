#include "cycling-sensor.h"

void CyclingSpeedAndCadence::onConnect()
{
    peer.discoverAllServices();
    for (auto& serv: peer.services()) {
        if (serv.UUID().type() == BleUuidType::SHORT) {
            switch (serv.UUID().shorted())
            {
            case BLE_SIG_UUID_CYCLING_SPEED_CADENCE_SVC:
                cscService = std::make_unique<CyclingSpeedAndCadenceService>(serv, peer);
                cscService->onConnect();
                cscService->setNewValueCallback(_onNewValue, this);
                break;
            case BLE_SIG_UUID_DEVICE_INFORMATION_SVC:
                disService = std::make_unique<DeviceInformationService>(serv, peer);
                disService->onConnect();
                break;
            case BLE_SIG_UUID_BATTERY_SVC:
                battService = std::make_unique<BatteryService>(serv, peer);
                battService->onConnect();
                battService->setNewValueCallback(_onNewValue, this);
                break; 
            default:
                break;
            }
        }
    }
}

void CyclingSpeedAndCadence::_onNewValue(BleUuid uuid, void* context) {
    CyclingSpeedAndCadence* ctx = (CyclingSpeedAndCadence *)context;
    if (uuid == BLE_SIG_CSC_MEASUREMENT_CHAR) {
        if (ctx->wheel_rev == 0) {
            ctx->wheel_rev = ctx->getWheelRotations();
            ctx->timed_event = ctx->getLastWheelEvent();
        } else {
            if (ctx->getLastWheelEvent() - ctx->timed_event > 0) {
                float meters = (ctx->_wheelmm)/1000.0 * (ctx->getWheelRotations()-ctx->wheel_rev);
                float hours = ((ctx->getLastWheelEvent()-ctx->timed_event)/1024.0)/3600;
                ctx->_speed = (uint16_t)( meters/hours );
                ctx->wheel_rev = ctx->getWheelRotations();
                ctx->timed_event = ctx->getLastWheelEvent();
            } else {
                ctx->_speed = std::max(0, ctx->_speed - 2000);
            }
        }
    }
    if (ctx->_callback != nullptr) {
        ctx->_callback(*ctx, uuid, ctx->_callbackContext);
    }
}

void CyclingSpeedAndCadence::setNewValueCallback(NewCyclingCallback callback, void* context) {
    _callback = callback;
    _callbackContext = context;
}