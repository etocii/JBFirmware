/*
 * This file is part of Cleanflight and Betaflight.
 *
 * Cleanflight and Betaflight are free software. You can redistribute
 * this software and/or modify this software under the terms of the
 * GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * Cleanflight and Betaflight are distributed in the hope that they
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software.
 *
 * If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "common/axis.h"
#include "common/filter.h"
#include "common/time.h"
#include "common/utils.h"

#include "drivers/accgyro/accgyro.h"
#include "drivers/bus.h"
#include "drivers/sensor.h"

#ifdef USE_DYN_NOTCH_FILTER
#include "flight/dyn_notch_filter.h"
#endif

#include "flight/pid.h"

#include "pg/pg.h"

#define LPF_MAX_HZ                      1000
#define DYN_LPF_MAX_HZ                  1000
#define DYN_LPF_UPDATE_DELAY_US         5000

#define GYRO_LPF1_TYPE_DEFAULT          LPF_1ST_ORDER
#define GYRO_LPF1_HZ_DEFAULT            100
#define GYRO_LPF1_DYN_MIN_HZ_DEFAULT    0
#define GYRO_LPF1_DYN_MAX_HZ_DEFAULT    0

#define GYRO_LPF2_TYPE_DEFAULT          LPF_NONE
#define GYRO_LPF2_HZ_DEFAULT            50


typedef enum gyroDetectionFlags_e {
    GYRO_NONE_MASK = 0,
    GYRO_1_MASK = BIT(0),
#if defined(USE_MULTI_GYRO)
    GYRO_2_MASK = BIT(1),
    GYRO_ALL_MASK = (GYRO_1_MASK | GYRO_2_MASK),
    GYRO_IDENTICAL_MASK = BIT(7), // All gyros are of the same hardware type
#endif
} gyroDetectionFlags_t;

typedef struct gyroCalibration_s {
    float sum[XYZ_AXIS_COUNT];
    stdev_t var[XYZ_AXIS_COUNT];
    int32_t cyclesRemaining;
} gyroCalibration_t;

typedef struct gyroSensor_s {
    gyroDev_t gyroDev;
    gyroCalibration_t calibration;
} gyroSensor_t;

typedef struct gyro_s {
    uint16_t sampleRateHz;
    uint16_t filterRateHz;
    uint16_t targetRateHz;

    uint32_t sampleLooptime;
    uint32_t filterLooptime;
    uint32_t targetLooptime;

    float scale;

    float gyroADC[XYZ_AXIS_COUNT];     // aligned, calibrated, scaled, but unfiltered data from the sensor(s)
    float gyroADCd[XYZ_AXIS_COUNT];    // downsampled gyro data
    float gyroADCf[XYZ_AXIS_COUNT];    // filtered gyro data

    gyroSensor_t gyroSensor1;
#ifdef USE_MULTI_GYRO
    gyroSensor_t gyroSensor2;
#endif

    gyroDev_t *rawSensorDev;           // pointer to the sensor providing the raw data for DEBUG_GYRO_RAW

    // gyro decimation filter stack
    biquadFilter_t decimator[XYZ_AXIS_COUNT][2];

    // gyro lowpass filters
    filter_t lowpassFilter[XYZ_AXIS_COUNT];
    filter_t lowpass2Filter[XYZ_AXIS_COUNT];

    // Notch filters
    filter_t notchFilter1[XYZ_AXIS_COUNT];
    filter_t notchFilter2[XYZ_AXIS_COUNT];

    uint16_t accSampleRateHz;
    uint8_t gyroToUse;
    uint8_t gyroDebugMode;

    bool gyroHasOverflowProtection;
    bool useDualGyroDebugging;

#ifdef USE_DYN_LPF
    bool dynLpfFilter;
    uint16_t dynLpfHz;
    uint16_t dynLpfMin;
    uint16_t dynLpfMax;
#endif

#ifdef USE_GYRO_OVERFLOW_CHECK
    uint8_t overflowAxisMask;
#endif

} gyro_t;

extern gyro_t gyro;
extern uint8_t activePidLoopDenom;
extern uint8_t activeFilterLoopDenom;

enum {
    GYRO_OVERFLOW_CHECK_NONE = 0,
    GYRO_OVERFLOW_CHECK_YAW,
    GYRO_OVERFLOW_CHECK_ALL_AXES
};

#define GYRO_CONFIG_USE_GYRO_1      0
#define GYRO_CONFIG_USE_GYRO_2      1
#define GYRO_CONFIG_USE_GYRO_BOTH   2

typedef struct gyroConfig_s {
    uint8_t gyroMovementCalibrationThreshold; // people keep forgetting that moving model while init results in wrong gyro offsets. and then they never reset gyro. so this is now on by default.
    uint8_t gyro_hardware_lpf;                // gyro DLPF setting
    uint8_t gyro_high_fsr;
    uint8_t gyro_rate_sync;
    uint8_t gyro_to_use;

    uint16_t gyro_decimation_hz;

    uint16_t gyro_lpf1_static_hz;
    uint16_t gyro_lpf2_static_hz;

    uint16_t gyro_soft_notch_hz_1;
    uint16_t gyro_soft_notch_cutoff_1;
    uint16_t gyro_soft_notch_hz_2;
    uint16_t gyro_soft_notch_cutoff_2;
    int16_t gyro_offset_yaw;
    uint8_t checkOverflow;

    // Lowpass primary/secondary
    uint8_t gyro_lpf1_type;
    uint8_t gyro_lpf2_type;

    uint16_t gyroCalibrationDuration;   // Gyro calibration duration in 1/100 second

    uint16_t gyro_lpf1_dyn_min_hz;
    uint16_t gyro_lpf1_dyn_max_hz;

    uint8_t gyrosDetected; // What gyros should detection be attempted for on startup. Automatically set on first startup.

} gyroConfig_t;

PG_DECLARE(gyroConfig_t, gyroConfig);

void gyroUpdate(void);
void gyroFiltering(timeUs_t currentTimeUs);
bool gyroGetAccumulationAverage(float *accumulation);
void gyroStartCalibration(bool isFirstArmingCalibration);
bool isFirstArmingGyroCalibrationRunning(void);
bool gyroIsCalibrationComplete(void);
void gyroReadTemperature(void);
int16_t gyroGetTemperature(void);
bool gyroOverflowDetected(void);
uint16_t gyroAbsRateDps(int axis);
#ifdef USE_DYN_LPF
void dynLpfUpdate(timeUs_t currentTimeUs);
#endif
