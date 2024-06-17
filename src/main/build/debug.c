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

#include <stdint.h>

#include "debug.h"

FAST_DATA_ZERO_INIT uint8_t debugMode;
FAST_DATA_ZERO_INIT uint8_t debugAxis;

FAST_DATA_ZERO_INIT int32_t debug[DEBUG_VALUE_COUNT];

FAST_DATA_ZERO_INIT uint32_t __timing[DEBUG_VALUE_COUNT];

#define DEBUG_NAME(x)  [DEBUG_ ## x] = #x

// Please ensure that names listed here match the enum values defined in 'debug.h'
const char * const debugModeNames[DEBUG_COUNT] = {
    DEBUG_NAME(NONE),
    DEBUG_NAME(CYCLETIME),
    DEBUG_NAME(BATTERY),
    DEBUG_NAME(GYRO_FILTERED),
    DEBUG_NAME(ACCELEROMETER),
    DEBUG_NAME(PIDLOOP),
    DEBUG_NAME(GYRO_SCALED),
    DEBUG_NAME(RC_COMMAND),
    DEBUG_NAME(RC_SETPOINT),
    DEBUG_NAME(ESC_SENSOR),
    DEBUG_NAME(SCHEDULER),
    DEBUG_NAME(STACK),
    DEBUG_NAME(ESC_SENSOR_DATA),
    DEBUG_NAME(ESC_SENSOR_FRAME),
    DEBUG_NAME(ALTITUDE),
    DEBUG_NAME(DYN_NOTCH),
    DEBUG_NAME(DYN_NOTCH_TIME),
    DEBUG_NAME(DYN_NOTCH_FREQ),
    DEBUG_NAME(RX_FRSKY_SPI),
    DEBUG_NAME(RX_SFHSS_SPI),
    DEBUG_NAME(GYRO_RAW),
    DEBUG_NAME(DUAL_GYRO_RAW),
    DEBUG_NAME(DUAL_GYRO_DIFF),
    DEBUG_NAME(MAX7456_SIGNAL),
    DEBUG_NAME(MAX7456_SPICLOCK),
    DEBUG_NAME(SBUS),
    DEBUG_NAME(FPORT),
    DEBUG_NAME(RANGEFINDER),
    DEBUG_NAME(RANGEFINDER_QUALITY),
    DEBUG_NAME(LIDAR_TF),
    DEBUG_NAME(ADC_INTERNAL),
    DEBUG_NAME(GOVERNOR),
    DEBUG_NAME(SDIO),
    DEBUG_NAME(CURRENT_SENSOR),
    DEBUG_NAME(USB),
    DEBUG_NAME(SMARTAUDIO),
    DEBUG_NAME(RTH),
    DEBUG_NAME(ITERM_RELAX),
    DEBUG_NAME(ACRO_TRAINER),
    DEBUG_NAME(SETPOINT),
    DEBUG_NAME(RX_SIGNAL_LOSS),
    DEBUG_NAME(RC_RAW),
    DEBUG_NAME(RC_DATA),
    DEBUG_NAME(DYN_LPF),
    DEBUG_NAME(RX_SPEKTRUM_SPI),
    DEBUG_NAME(DSHOT_RPM_TELEMETRY),
    DEBUG_NAME(RPM_FILTER),
    DEBUG_NAME(RPM_SOURCE),
    DEBUG_NAME(TTA),
    DEBUG_NAME(AIRBORNE),
    DEBUG_NAME(DUAL_GYRO_SCALED),
    DEBUG_NAME(DSHOT_RPM_ERRORS),
    DEBUG_NAME(CRSF_LINK_STATISTICS_UPLINK),
    DEBUG_NAME(CRSF_LINK_STATISTICS_PWR),
    DEBUG_NAME(CRSF_LINK_STATISTICS_DOWN),
    DEBUG_NAME(BARO),
    DEBUG_NAME(GPS_RESCUE_THROTTLE_PID),
    DEBUG_NAME(FREQ_SENSOR),
    DEBUG_NAME(FEEDFORWARD_LIMIT),
    DEBUG_NAME(FEEDFORWARD),
    DEBUG_NAME(BLACKBOX_OUTPUT),
    DEBUG_NAME(GYRO_SAMPLE),
    DEBUG_NAME(RX_TIMING),
    DEBUG_NAME(D_LPF),
    DEBUG_NAME(VTX_TRAMP),
    DEBUG_NAME(GHST),
    DEBUG_NAME(SCHEDULER_DETERMINISM),
    DEBUG_NAME(TIMING_ACCURACY),
    DEBUG_NAME(RX_EXPRESSLRS_SPI),
    DEBUG_NAME(RX_EXPRESSLRS_PHASELOCK),
    DEBUG_NAME(RX_STATE_TIME),
    DEBUG_NAME(PITCH_PRECOMP),
    DEBUG_NAME(YAW_PRECOMP),
    DEBUG_NAME(RESCUE),
    DEBUG_NAME(RESCUE_ALTHOLD),
    DEBUG_NAME(CROSS_COUPLING),
    DEBUG_NAME(ERROR_DECAY),
    DEBUG_NAME(HS_OFFSET),
    DEBUG_NAME(HS_BLEED),
};