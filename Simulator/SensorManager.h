#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <vector>
#include <memory>
#include <atomic>
#include <array>
#include <mutex>
#include "Sensor.h"

class SensorManager {
public:
    SensorManager(std::vector<std::unique_ptr<Sensor>> sensorsInit)
        : sensors(std::move(sensorsInit))
    {
        for (auto& reading : readings) {
            reading.store(SENSOR_INIT);
        }
    }

    // Delete copy/move constructors to prevent copying unique_ptrs
    SensorManager(const SensorManager&) = delete;
    SensorManager& operator=(const SensorManager&) = delete;
    SensorManager(SensorManager&&) = delete;
    SensorManager& operator=(SensorManager&&) = delete;

    // Thread-safe update for sensor at index
    void updateSensor(size_t index) {
        if (index >= sensors.size()) return;
        float value = sensors[index]->next();
        readings[index].store(value);
    }

    // Thread-safe get reading at index
    float getReading(size_t index) const {
        if (index >= readings.size()) return SENSOR_INIT;
        return readings[index].load();
    }

    void addSensor(float mean, float stddev) {
        sensors.emplace_back(std::make_unique<Sensor>(mean, stddev));
    }

    Sensor& getSensor(size_t index) {
        return *sensors.at(index);
    }

private:
    std::vector<std::unique_ptr<Sensor>> sensors;
    std::vector<std::atomic<float>> readings;
};

#endif // SENSOR_MANAGER_H