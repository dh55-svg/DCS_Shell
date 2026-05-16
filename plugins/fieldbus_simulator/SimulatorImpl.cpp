#include "SimulatorImpl.h"
#include "plugin_interface/IMessageBus.h"
#include <QDebug>
SimulatorImpl::SimulatorImpl(QObject *parent)
    : IFieldbus{parent}
{
    connect(&m_timer, &QTimer::timeout, this, &SimulatorImpl::onTick);
}
SimulatorImpl::~SimulatorImpl() {
    stopAll();
}
void SimulatorImpl::addDevice(const DeviceConfig& cfg) {
    SimDevice dev;
    dev.config = cfg;
    for (int i = 0; i < cfg.regCount; ++i) {
        float phase = (float)i * 0.3f + (float)cfg.deviceId * 1.2f;
        dev.tagPhases.append({cfg.regStart + i, phase});
    }
    m_devices.append(dev);
}
void SimulatorImpl::removeDevice(int devId) {
    for (int i = 0; i < m_devices.size(); ++i) {
        if (m_devices[i].config.deviceId == devId) {
            m_devices.removeAt(i);
            return;
        }
    }
}
void SimulatorImpl::startAll() {
    for (auto& dev : m_devices) dev.running = true;
    m_timer.start(500);
}

void SimulatorImpl::stopAll() {
    for (auto& dev : m_devices) dev.running = false;
    m_timer.stop();
}

bool SimulatorImpl::isDeviceConnected(int devId) const {
    for (auto& dev : m_devices) {
        if (dev.config.deviceId == devId) return dev.running;
    }
    return false;
}

int SimulatorImpl::onlineDeviceCount() const {
    int count = 0;
    for (auto& dev : m_devices) { if (dev.running) count++; }
    return count;
}

int SimulatorImpl::totalDeviceCount() const {
    return m_devices.size();
}

void SimulatorImpl::writeRegister(int devId, int addr, quint16 val) {
    // Demo mode: writes are no-ops
    Q_UNUSED(devId);
    Q_UNUSED(addr);
    Q_UNUSED(val);
}

void SimulatorImpl::setDataSink(IMessageBus* sink) {
    m_sink = sink;
}

void SimulatorImpl::onTick() {
    if (!m_sink) return;
    m_tick++;

    for (auto& dev : m_devices) {
        if (!dev.running) continue;
        RawModbusData raw;
        raw.serverAddr = dev.config.serverAddr;
        raw.startAddr = dev.config.regStart;
        raw.count = dev.config.regCount;

        for (int i = 0; i < dev.tagPhases.size(); ++i) {
            float phase = dev.tagPhases[i].second;
            // Primary wave: 40 amplitude sweeps 10-90, triggering High/Low regularly
            float val = 50.0f + 40.0f * qSin((m_tick * 0.1f) + phase);
            // Secondary wave for Critical zone excursions (0-100 range)
            val += 12.0f * qSin((m_tick * 0.037f) + phase * 2.5f);
            // Noise for jitter
            val += 3.0f * qSin((m_tick * 0.23f) + phase * 5.7f);
            quint16 rawVal = (quint16)(qBound(0.0f, val / 100.0f, 1.0f) * 65535.0f);
            if (i < 128) raw.values[i] = rawVal;
        }
        m_sink->enqueue(raw);
    }
}

QVector<DeviceConfig> SimulatorImpl::allDeviceConfigs() const {
    QVector<DeviceConfig> configs;
    for (const auto& dev : m_devices)
        configs.append(dev.config);
    return configs;
}

QJsonObject SimulatorImpl::statusSnapshot() const {
    QJsonObject obj;
    obj["protocol"] = "Simulator";
    obj["devices"] = totalDeviceCount();
    obj["online"] = onlineDeviceCount();
    return obj;
}
