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

#include "platform.h"
#include "build/build_config.h"
#include "common/time.h"
#include "pg/pg.h"

typedef enum BlackboxDevice {
    BLACKBOX_DEVICE_NONE = 0,
    BLACKBOX_DEVICE_FLASH = 1,
    BLACKBOX_DEVICE_SDCARD = 2,
    BLACKBOX_DEVICE_SERIAL = 3
} BlackboxDevice_e;

typedef enum BlackboxMode {
    BLACKBOX_MODE_OFF = 0,
    BLACKBOX_MODE_NORMAL,
    BLACKBOX_MODE_ARMED,
    BLACKBOX_MODE_SWITCH,
} BlackboxMode;

typedef enum FlightLogEvent {
    FLIGHT_LOG_EVENT_SYNC_BEEP = 0,
    FLIGHT_LOG_EVENT_INFLIGHT_ADJUSTMENT = 13,
    FLIGHT_LOG_EVENT_LOGGING_RESUME = 14,
    FLIGHT_LOG_EVENT_DISARM = 15,
    FLIGHT_LOG_EVENT_FLIGHTMODE = 30, // Add new event type for flight mode status.
    FLIGHT_LOG_EVENT_GOVSTATE = 50,   // Add new event type for main motor governor state.
    FLIGHT_LOG_EVENT_RESCUE_STATE = 51,
    FLIGHT_LOG_EVENT_AIRBORNE_STATE = 52,
    FLIGHT_LOG_EVENT_CUSTOM_DATA = 100,
    FLIGHT_LOG_EVENT_CUSTOM_STRING = 101,
    FLIGHT_LOG_EVENT_LOG_END = 255
} FlightLogEvent;

typedef struct blackboxConfig_s {
    uint8_t device;
    uint8_t mode;
    uint16_t denom;
    uint32_t fields;
} blackboxConfig_t;

PG_DECLARE(blackboxConfig_t, blackboxConfig);

union flightLogEventData_u;
void blackboxLogEvent(FlightLogEvent event, union flightLogEventData_u *data);

void blackboxLogCustomData(const uint8_t *ptr, size_t length);
void blackboxLogCustomString(const char *ptr);

void blackboxUpdate(timeUs_t currentTimeUs);
void blackboxFlush(timeUs_t currentTimeUs);
void blackboxInit(void);

void blackboxErase(void);
bool isBlackboxErased(void);

void blackboxSetStartDateTime(const char *dateTime, timeMs_t timeNowMs);
void blackboxValidateConfig(void);
bool blackboxMayEditConfig(void);

#ifdef UNIT_TEST
STATIC_UNIT_TESTED void blackboxLogIteration(timeUs_t currentTimeUs);
STATIC_UNIT_TESTED bool blackboxShouldLogPFrame(void);
STATIC_UNIT_TESTED bool blackboxShouldLogIFrame(void);
STATIC_UNIT_TESTED bool blackboxShouldLogGpsHomeFrame(void);
STATIC_UNIT_TESTED bool writeSlowFrameIfNeeded(void);
// Called once every FC loop in order to keep track of how many FC loop iterations have passed
STATIC_UNIT_TESTED void blackboxAdvanceIterationTimers(void);
extern int32_t blackboxSInterval;
extern int32_t blackboxSlowFrameIterationTimer;
#endif
