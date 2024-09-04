// SPDX-License-Identifier: GPL-2.0-or-later

package org.dolphinemu.dolphinemu.features.input.model

import android.content.Context
import android.hardware.input.InputManager
import android.os.Handler
import android.os.Vibrator
import android.view.InputDevice
import android.view.KeyEvent
import android.view.MotionEvent
import androidx.annotation.Keep
import org.dolphinemu.dolphinemu.DolphinApplication
import org.dolphinemu.dolphinemu.utils.HapticEffect
import org.dolphinemu.dolphinemu.utils.HapticsProvider
import org.dolphinemu.dolphinemu.utils.LooperThread

/**
 * This class interfaces with the native ControllerInterface,
 * which is where the emulator core gets inputs from.
 */
object ControllerInterface {
    private var inputDeviceListener: InputDeviceListener? = null
    private lateinit var looperThread: LooperThread

    /**
     * Activities which want to pass on inputs to native code
     * should call this in their own dispatchKeyEvent method.
     *
     * @return true if the emulator core seems to be interested in this event.
     * false if the event should be passed on to the default dispatchKeyEvent.
     */
    external fun dispatchKeyEvent(event: KeyEvent): Boolean

    /**
     * Activities which want to pass on inputs to native code
     * should call this in their own dispatchGenericMotionEvent method.
     *
     * @return true if the emulator core seems to be interested in this event.
     * false if the event should be passed on to the default dispatchGenericMotionEvent.
     */
    external fun dispatchGenericMotionEvent(event: MotionEvent): Boolean

    /**
     * [DolphinSensorEventListener] calls this for each axis of a received SensorEvent.
     *
     * @return true if the emulator core seems to be interested in this event.
     * false if the sensor can be suspended to save battery.
     */
    external fun dispatchSensorEvent(
        deviceQualifier: String,
        axisName: String,
        value: Float
    ): Boolean

    /**
     * Called when a sensor is suspended or unsuspended.
     *
     * @param deviceQualifier A string used by native code for uniquely identifying devices.
     * @param axisNames       The name of all axes for the sensor.
     * @param suspended       Whether the sensor is now suspended.
     */
    external fun notifySensorSuspendedState(
        deviceQualifier: String,
        axisNames: Array<String>,
        suspended: Boolean
    )

    /**
     * Rescans for input devices.
     */
    external fun refreshDevices()

    external fun getAllDeviceStrings(): Array<String>

    external fun getDevice(deviceString: String): CoreDevice?

    @Keep
    @JvmStatic
    private fun registerInputDeviceListener() {
        looperThread = LooperThread("Hotplug thread")
        looperThread.start()

        if (inputDeviceListener == null) {
            val im = DolphinApplication.getAppContext()
                .getSystemService(Context.INPUT_SERVICE) as InputManager?

            inputDeviceListener = InputDeviceListener()
            im!!.registerInputDeviceListener(inputDeviceListener, Handler(looperThread.looper))
        }
    }

    @Keep
    @JvmStatic
    private fun unregisterInputDeviceListener() {
        if (inputDeviceListener != null) {
            val im = DolphinApplication.getAppContext()
                .getSystemService(Context.INPUT_SERVICE) as InputManager?

            im!!.unregisterInputDeviceListener(inputDeviceListener)
            inputDeviceListener = null
        }
    }

    @Keep
    @JvmStatic
    private fun getDeviceVibratorManager(device: InputDevice): DolphinVibratorManager =
        DolphinVibratorManagerFactory.getDeviceVibratorManager(device)

    @Keep
    @JvmStatic
    private fun getSystemVibratorManager(): DolphinVibratorManager =
        DolphinVibratorManagerFactory.getSystemVibratorManager()

    @Keep
    @JvmStatic
    private fun vibrate(vibrator: Vibrator) {
        // TODO: Add a slider to the Rumble options that allows adjusting the vibration intensity.
        HapticsProvider(vibrator).provideFeedback(HapticEffect.SPIN, 0.5f)
    }

    private class InputDeviceListener : InputManager.InputDeviceListener {
        // Simple implementation for now. We could do something fancier if we wanted to.
        override fun onInputDeviceAdded(deviceId: Int) = refreshDevices()

        // Simple implementation for now. We could do something fancier if we wanted to.
        override fun onInputDeviceRemoved(deviceId: Int) = refreshDevices()

        // Simple implementation for now. We could do something fancier if we wanted to.
        override fun onInputDeviceChanged(deviceId: Int) = refreshDevices()
    }
}
