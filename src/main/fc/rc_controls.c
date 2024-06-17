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

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <math.h>

#include "platform.h"

#include "blackbox/blackbox.h"

#include "build/build_config.h"

#include "common/axis.h"
#include "common/maths.h"

#include "config/feature.h"

#include "drivers/camera_control.h"

#include "config/config.h"
#include "fc/core.h"
#include "fc/rc.h"
#include "fc/runtime_config.h"

#include "flight/pid.h"
#include "flight/failsafe.h"

#include "io/beeper.h"
#include "io/usb_cdc_hid.h"
#include "io/dashboard.h"
#include "io/gps.h"
#include "io/vtx_control.h"

#include "pg/pg.h"
#include "pg/pg_ids.h"
#include "pg/rx.h"

#include "rx/rx.h"

#include "scheduler/scheduler.h"

#include "sensors/acceleration.h"
#include "sensors/barometer.h"
#include "sensors/battery.h"
#include "sensors/compass.h"
#include "sensors/gyro.h"

#include "rc_controls.h"


#define STICK_COMMAND_MIN -450
#define STICK_COMMAND_MAX  450

#define ROL_LO    (1 << 0)
#define ROL_HI    (2 << 0)
#define ROL_CE    (3 << 0)
#define PIT_LO    (1 << 2)
#define PIT_HI    (2 << 2)
#define PIT_CE    (3 << 2)
#define YAW_LO    (1 << 4)
#define YAW_HI    (2 << 4)
#define YAW_CE    (3 << 4)
#define COL_LO    (1 << 6)
#define COL_HI    (2 << 6)
#define COL_CE    (3 << 6)
#define COL_MASK  (3 << 6)


// true if arming is done via the sticks (as opposed to a switch)
static bool isUsingStickArming = false;
static bool isUsingStickCommands = false;

PG_REGISTER_WITH_RESET_TEMPLATE(armingConfig_t, armingConfig, PG_ARMING_CONFIG, 1);

PG_RESET_TEMPLATE(armingConfig_t, armingConfig,
    .gyro_cal_on_first_arm = 0,  // TODO - Cleanup retarded arm support
    .auto_disarm_delay = 5
);

bool isUsingSticksForArming(void)
{
    return isUsingStickArming;
}

bool areSticksInApModePosition(uint16_t ap_mode)
{
    return fabsf(rcCommand[ROLL]) < ap_mode && fabsf(rcCommand[PITCH]) < ap_mode;
}

#define ARM_DELAY_MS        500
#define STICK_DELAY_MS      50
#define STICK_AUTOREPEAT_MS 250

#define repeatAfter(t) do { \
    rcDelayMs -= (t); \
    doNotRepeat = false; \
} while (0)

void processRcStickPositions()
{
    // time the sticks are maintained
    static int16_t rcDelayMs;
    // hold sticks position for command combos
    static uint8_t rcSticks;
    // an extra guard for disarming through switch to prevent that one frame can disarm it
    static uint8_t rcDisarmTicks;
    static bool doNotRepeat;
    static bool pendingApplyRollAndPitchTrimDeltaSave = false;

    // checking sticks positions
    uint8_t stTmp = 0;
    for (int i = 0; i < 4; i++) {
        stTmp >>= 2;
        if (rcCommand[i] > STICK_COMMAND_MIN) {
            stTmp |= 0x80;  // check for MIN
        }
        if (rcCommand[i] < STICK_COMMAND_MAX) {
            stTmp |= 0x40;  // check for MAX
        }
    }
    if (stTmp == rcSticks) {
        if (rcDelayMs <= INT16_MAX - (getTaskDeltaTimeUs(TASK_SELF) / 1000)) {
            rcDelayMs += getTaskDeltaTimeUs(TASK_SELF) / 1000;
        }
    } else {
        rcDelayMs = 0;
        doNotRepeat = false;
    }
    rcSticks = stTmp;

    // perform actions
    if (!isUsingStickArming) {
        if (IS_RC_MODE_ACTIVE(BOXARM)) {
            rcDisarmTicks = 0;
            // Arming via ARM BOX
            tryArm();
        } else {
            resetTryingToArm();
            // Disarming via ARM BOX
            resetArmingDisabled();
            if (ARMING_FLAG(ARMED) && rxIsReceivingSignal() && !failsafeIsActive()  ) {
                rcDisarmTicks++;
                if (rcDisarmTicks > 3) {
                    disarm(DISARM_REASON_SWITCH);
                }
            }
        }
    } else if (rcSticks == COL_LO + YAW_LO + PIT_CE + ROL_CE) {
        if (rcDelayMs >= ARM_DELAY_MS && !doNotRepeat) {
            doNotRepeat = true;
            // Disarm on throttle down + yaw
            resetTryingToArm();
            if (ARMING_FLAG(ARMED))
                disarm(DISARM_REASON_STICKS);
            else {
                beeper(BEEPER_DISARM_REPEAT);     // sound tone while stick held
                repeatAfter(STICK_AUTOREPEAT_MS); // disarm tone will repeat
            }
        }
        return;
    } else if (rcSticks == COL_LO + YAW_HI + PIT_CE + ROL_CE && !IS_RC_MODE_ACTIVE(BOXSTICKCOMMANDDISABLE)) { // disable stick arming if STICK COMMAND DISABLE SW is active
        if (rcDelayMs >= ARM_DELAY_MS && !doNotRepeat) {
            doNotRepeat = true;
            if (!ARMING_FLAG(ARMED)) {
                // Arm via YAW
                tryArm();
                if (isTryingToArm() ||
                    ((getArmingDisableFlags() == ARMING_DISABLED_CALIBRATING) && armingConfig()->gyro_cal_on_first_arm)) {
                    doNotRepeat = false;
                }
            } else {
                resetArmingDisabled();
            }
        }
        return;
    } else {
        resetTryingToArm();
    }

    if (ARMING_FLAG(ARMED) || doNotRepeat || rcDelayMs <= STICK_DELAY_MS) {
        return;
    }
    doNotRepeat = true;

#ifdef USE_USB_CDC_HID
    // If this target is used as a joystick, we should leave here.
    if (cdcDeviceIsMayBeActive() || IS_RC_MODE_ACTIVE(BOXSTICKCOMMANDDISABLE)) {
        return;
    }
#endif

    // Stick commands in use
    if (!isUsingStickCommands)
        return;

    // actions during not armed
    if (rcSticks == COL_LO + YAW_LO + PIT_LO + ROL_CE) {
        // GYRO calibration
        gyroStartCalibration(false);

#ifdef USE_GPS
        if (featureIsEnabled(FEATURE_GPS)) {
            GPS_reset_home_position();
        }
#endif

#ifdef USE_BARO
        if (sensors(SENSOR_BARO)) {
            baroSetGroundLevel();
        }
#endif
        return;
    }

    // Change PID profile
    switch (rcSticks) {
    case COL_LO + YAW_LO + PIT_CE + ROL_LO:
        // ROLL left -> PID profile 1
        changePidProfile(0);
        return;
    case COL_LO + YAW_LO + PIT_HI + ROL_CE:
        // PITCH up -> PID profile 2
        changePidProfile(1);
        return;
    case COL_LO + YAW_LO + PIT_CE + ROL_HI:
        // ROLL right -> PID profile 3
        changePidProfile(2);
        return;
    }

    if (rcSticks == COL_LO + YAW_LO + PIT_LO + ROL_HI) {
        saveConfigAndNotify();
    }

#ifdef USE_ACC
    if (rcSticks == COL_HI + YAW_LO + PIT_LO + ROL_CE) {
        // Calibrating Acc
        accStartCalibration();
        return;
    }
#endif

#if defined(USE_MAG)
    if (rcSticks == COL_HI + YAW_HI + PIT_LO + ROL_CE) {
        // Calibrating Mag
        compassStartCalibration();
        return;
    }
#endif


    if (FLIGHT_MODE(ANGLE_MODE | HORIZON_MODE)) {
        // in ANGLE or HORIZON mode, so use sticks to apply accelerometer trims
        rollAndPitchTrims_t accelerometerTrimsDelta;
        memset(&accelerometerTrimsDelta, 0, sizeof(accelerometerTrimsDelta));

        if (pendingApplyRollAndPitchTrimDeltaSave && ((rcSticks & COL_MASK) != COL_HI)) {
            saveConfigAndNotify();
            pendingApplyRollAndPitchTrimDeltaSave = false;
            return;
        }

        bool shouldApplyRollAndPitchTrimDelta = false;
        switch (rcSticks) {
        case COL_HI + YAW_CE + PIT_HI + ROL_CE:
            accelerometerTrimsDelta.values.pitch = 1;
            shouldApplyRollAndPitchTrimDelta = true;
            break;
        case COL_HI + YAW_CE + PIT_LO + ROL_CE:
            accelerometerTrimsDelta.values.pitch = -1;
            shouldApplyRollAndPitchTrimDelta = true;
            break;
        case COL_HI + YAW_CE + PIT_CE + ROL_HI:
            accelerometerTrimsDelta.values.roll = 1;
            shouldApplyRollAndPitchTrimDelta = true;
            break;
        case COL_HI + YAW_CE + PIT_CE + ROL_LO:
            accelerometerTrimsDelta.values.roll = -1;
            shouldApplyRollAndPitchTrimDelta = true;
            break;
        }
        if (shouldApplyRollAndPitchTrimDelta) {
#if defined(USE_ACC)
            applyAccelerometerTrimsDelta(&accelerometerTrimsDelta);
#endif
            pendingApplyRollAndPitchTrimDeltaSave = true;

            beeperConfirmationBeeps(1);

            repeatAfter(STICK_AUTOREPEAT_MS);

            return;
        }
    } else {
        // in ACRO mode, so use sticks to change RATE profile
        switch (rcSticks) {
        case COL_HI + YAW_CE + PIT_HI + ROL_CE:
            changeControlRateProfile(0);
            return;
        case COL_HI + YAW_CE + PIT_LO + ROL_CE:
            changeControlRateProfile(1);
            return;
        case COL_HI + YAW_CE + PIT_CE + ROL_HI:
            changeControlRateProfile(2);
            return;
        case COL_HI + YAW_CE + PIT_CE + ROL_LO:
            changeControlRateProfile(3);
            return;
        }
    }

#ifdef USE_DASHBOARD
    if (rcSticks == COL_LO + YAW_CE + PIT_HI + ROL_LO) {
        dashboardDisablePageCycling();
    }

    if (rcSticks == COL_LO + YAW_CE + PIT_HI + ROL_HI) {
        dashboardEnablePageCycling();
    }
#endif

#ifdef USE_VTX_CONTROL
    if (rcSticks ==  COL_HI + YAW_LO + PIT_CE + ROL_HI) {
        vtxIncrementBand();
    }
    if (rcSticks ==  COL_HI + YAW_LO + PIT_CE + ROL_LO) {
        vtxDecrementBand();
    }
    if (rcSticks ==  COL_HI + YAW_HI + PIT_CE + ROL_HI) {
        vtxIncrementChannel();
    }
    if (rcSticks ==  COL_HI + YAW_HI + PIT_CE + ROL_LO) {
        vtxDecrementChannel();
    }
#endif

#ifdef USE_CAMERA_CONTROL
    if (rcSticks == COL_CE + YAW_HI + PIT_CE + ROL_CE) {
        cameraControlKeyPress(CAMERA_CONTROL_KEY_ENTER, 0);
        repeatAfter(3 * STICK_DELAY_MS);
    } else if (rcSticks == COL_CE + YAW_CE + PIT_CE + ROL_LO) {
        cameraControlKeyPress(CAMERA_CONTROL_KEY_LEFT, 0);
        repeatAfter(3 * STICK_DELAY_MS);
    } else if (rcSticks == COL_CE + YAW_CE + PIT_HI + ROL_CE) {
        cameraControlKeyPress(CAMERA_CONTROL_KEY_UP, 0);
        repeatAfter(3 * STICK_DELAY_MS);
    } else if (rcSticks == COL_CE + YAW_CE + PIT_CE + ROL_HI) {
        cameraControlKeyPress(CAMERA_CONTROL_KEY_RIGHT, 0);
        repeatAfter(3 * STICK_DELAY_MS);
    } else if (rcSticks == COL_CE + YAW_CE + PIT_LO + ROL_CE) {
        cameraControlKeyPress(CAMERA_CONTROL_KEY_DOWN, 0);
        repeatAfter(3 * STICK_DELAY_MS);
    } else if (rcSticks == COL_LO + YAW_CE + PIT_HI + ROL_CE) {
        cameraControlKeyPress(CAMERA_CONTROL_KEY_UP, 2000);
    }
#endif
}

void rcControlsInit(void)
{
    analyzeModeActivationConditions();
    isUsingStickArming = !isModeActivationConditionPresent(BOXARM) && systemConfig()->enableStickArming;
    isUsingStickCommands = systemConfig()->enableStickCommands;
}
