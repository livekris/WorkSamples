// Sensor.h

#ifndef SENSOR_H
#define SENSOR_H

#include <random>

// Temperature range
static constexpr float MIN_TEMP = 30.0f;
static constexpr float MAX_TEMP = 70.0f;

static constexpr float SENSOR_INIT = 50.0f;

// Constants for sensor indexing
constexpr std::size_t SENSOR_1 = 0;
constexpr std::size_t SENSOR_2 = 1;
constexpr std::size_t SENSOR_COUNT = 2;

// Sensor class with next() generator function to produce sensor readings
class Sensor 
{
public:
    Sensor(float mean, float stddev, int faultRate = DICE_N) 
        : dist(mean, stddev),
          dice(1, faultRate),
          lowSpikeDist(SPIKE_LOW_MIN, SPIKE_LOW_MAX),
          highSpikeDist(SPIKE_HIGH_MIN, SPIKE_HIGH_MAX) 
    {
            std::random_device rd;
            gen.seed(rd());
    }

    float next() 
    {
        return (dice(gen) == 1 ? faulty() : dist(gen)); // Returns a normal range (with occassional spike)
    }

    float faulty() 
    {
        return spike(dice(gen) % 2); // Either returns a low or high spike
    }

    static constexpr int DICE_N = 10; // n sided dice

private:
    std::mt19937 gen; // Random number generator engine for this sensor
    std::normal_distribution<float> dist; // Normal distribution for sensor readings

    std::uniform_int_distribution<int> dice; // Selects if to use a fault spike or generate normal
    std::uniform_real_distribution<float> lowSpikeDist; // Distribution to spike normal sensor reading (low)
    std::uniform_real_distribution<float> highSpikeDist; // Distribution to spike normal sensor reading (high)

    // Temperature spike generation ranges
    static constexpr float SPIKE_LOW_MIN = 10.0f;
    static constexpr float SPIKE_LOW_MAX = MIN_TEMP - 1.0f;  // e.g. 30.0f - 1.0f = 29.0f
    static constexpr float SPIKE_HIGH_MIN = MAX_TEMP + 1.0f; // e.g. 70.0f + 1.0f = 71.0f
    static constexpr float SPIKE_HIGH_MAX = 90.0f;

    // Randomly spikes the normal data to test faulty sensors: return either a high or low fault spike based on `highSpike` flag
    // Can be customized to simulate only faulty data by changing DICE_N=2
    float spike(bool highSpike) 
    { 
        return highSpike ? highSpikeDist(gen) : lowSpikeDist(gen);
    }
};

#endif // SENSOR_H
