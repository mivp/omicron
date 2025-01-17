// vrpn_XInputGamepad.C: Gamepad driver for devices using XInput
// such as (read: primarily) the Microsoft Xbox 360 controller.

#include "omicron/VRPNDevice.h"

#include <xinput.h>
#pragma comment(lib,"XInput.lib")
using namespace omicron;

vrpn_XInputGamepad::vrpn_XInputGamepad(const char *name, vrpn_Connection *c, unsigned int controllerIndex):
	vrpn_Analog(name, c),
	vrpn_Button(name, c),
	vrpn_Analog_Output(name, c),
	_controllerIndex(controllerIndex)
{
	vrpn_Analog::num_channel = 7;
	vrpn_Button::num_buttons = 10;
	vrpn_Analog_Output::o_num_channel = 2;

	_motorSpeed[0] = 0;
	_motorSpeed[1] = 0;

	if (register_autodeleted_handler(
	 d_connection->register_message_type(vrpn_dropped_last_connection),
	 handle_last_connection_dropped, this)) {
		fprintf(stderr, "vrpn_XInputGamepad: Can't register connections-dropped handler\n");
		return;
	}
}

vrpn_XInputGamepad::~vrpn_XInputGamepad() {
}

void vrpn_XInputGamepad::mainloop() {
	XINPUT_STATE state;
	DWORD rv;

	server_mainloop();
	if ((rv = XInputGetState(_controllerIndex, &state)) != ERROR_SUCCESS) {
		char errMsg[256];
		struct timeval now;

		if (rv == ERROR_DEVICE_NOT_CONNECTED)
			sprintf(errMsg, "XInput device %u not connected", _controllerIndex);
		else
			sprintf(errMsg, "XInput device %u returned Windows error code %u",
				_controllerIndex, rv);

		vrpn_gettimeofday(&now, NULL);
		send_text_message(errMsg, now, vrpn_TEXT_ERROR);
		return;
	}

	// Set device state in VRPN_Analog
	channel[0] = normalize_axis(state.Gamepad.sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
	channel[1] = normalize_axis(state.Gamepad.sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
	channel[2] = normalize_axis(state.Gamepad.sThumbRX, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
	channel[3] = normalize_axis(state.Gamepad.sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
	channel[4] = normalize_dpad(state.Gamepad.wButtons);
	channel[5] = normalize_trigger(state.Gamepad.bLeftTrigger);
	channel[6] = normalize_trigger(state.Gamepad.bRightTrigger);

	// Set device state in VRPN_Button
	// Buttons are listed in DirectInput ordering
	buttons[0] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_A) != 0;
	buttons[1] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_B) != 0;
	buttons[2] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_X) != 0;
	buttons[3] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_Y) != 0;
	buttons[4] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0;
	buttons[5] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0;
	buttons[6] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK) != 0;
	buttons[7] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_START) != 0;
	buttons[8] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) != 0;
	buttons[9] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) != 0;

	vrpn_gettimeofday(&_timestamp, NULL);
	report_changes();
}

void vrpn_XInputGamepad::update(const Event* evt) {

	server_mainloop();
	/*if ((rv = XInputGetState(_controllerIndex, &state)) != ERROR_SUCCESS) {
		char errMsg[256];
		struct timeval now;

		if (rv == ERROR_DEVICE_NOT_CONNECTED)
			sprintf(errMsg, "XInput device %u not connected", _controllerIndex);
		else
			sprintf(errMsg, "XInput device %u returned Windows error code %u",
				_controllerIndex, rv);

		vrpn_gettimeofday(&now, NULL);
		send_text_message(errMsg, now, vrpn_TEXT_ERROR);
		return;
	}

	// Set device state in VRPN_Analog
	channel[0] = normalize_axis(state.Gamepad.sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
	channel[1] = normalize_axis(state.Gamepad.sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
	channel[2] = normalize_axis(state.Gamepad.sThumbRX, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
	channel[3] = normalize_axis(state.Gamepad.sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
	channel[4] = normalize_dpad(state.Gamepad.wButtons);
	channel[5] = normalize_trigger(state.Gamepad.bLeftTrigger);
	channel[6] = normalize_trigger(state.Gamepad.bRightTrigger);

	// Set device state in VRPN_Button
	// Buttons are listed in DirectInput ordering
	buttons[0] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_A) != 0;
	buttons[1] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_B) != 0;
	buttons[2] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_X) != 0;
	buttons[3] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_Y) != 0;
	buttons[4] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0;
	buttons[5] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0;
	buttons[6] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK) != 0;
	buttons[7] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_START) != 0;
	buttons[8] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) != 0;
	buttons[9] = (state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) != 0;
	*/
	
	// Assuming PS3 Navigator Controller
	// Left mouse = Cross
	// Right mouse = Circle
	// Middle mouse = Analog button
	// Mouse wheel = Analog up/down
	if( evt->getServiceType() == Event::ServiceTypeController || evt->getServiceType() == Event::ServiceTypeWand ){
		buttons[0] = (evt->getFlags() & Event::Button3) == Event::Button3;
		buttons[1] = (evt->getFlags() & Event::Button2) == Event::Button2;
		buttons[2] = (evt->getFlags() & Event::Button6) == Event::Button6;
		buttons[3] = (evt->getFlags() & Event::ButtonUp) == Event::ButtonUp;
		buttons[4] = (evt->getFlags() & Event::ButtonDown) == Event::ButtonDown;
		buttons[5] = (evt->getFlags() & Event::ButtonLeft) == Event::ButtonLeft;
		buttons[6] = (evt->getFlags() & Event::ButtonRight) == Event::ButtonRight;
		buttons[7] = (evt->getFlags() & Event::Button5) == Event::Button5;
		
		// Analog 4 (Wand L2)
		if( !evt->isExtraDataNull(4) && evt->getExtraDataFloat(4) > 0.5 )
			buttons[2] = 1;
		else if( buttons[2] != 1 )
			buttons[2] = 0;
			
		if( !evt->isExtraDataNull(0) )
			channel[0] = evt->getExtraDataFloat(0);
		if( !evt->isExtraDataNull(1) )
			channel[1] = evt->getExtraDataFloat(1);
	}

	vrpn_gettimeofday(&_timestamp, NULL);
	report_changes();
}

vrpn_float64 vrpn_XInputGamepad::normalize_dpad(WORD buttons) const {
	int x = 0;
	int y = 0;

	if (buttons & XINPUT_GAMEPAD_DPAD_RIGHT)
		x += 1;
	if (buttons & XINPUT_GAMEPAD_DPAD_LEFT)
		x -= 1;
	if (buttons & XINPUT_GAMEPAD_DPAD_UP)
		y += 1;
	if (buttons & XINPUT_GAMEPAD_DPAD_DOWN)
		y -= 1;

	size_t index = (x + 1) * 3 + (y + 1);
	vrpn_float64 angles[] = {225, 270, 315, 180, -1, 0, 135, 90, 45};
	return angles[index];
}


vrpn_float64 vrpn_XInputGamepad::normalize_axis(SHORT axis, SHORT deadzone) const {
	// Filter out areas near the center
	if (axis > -deadzone && axis < deadzone)
		return 0;

	// Note ranges are asymmetric (-32768 to 32767)
	return axis / ((axis < 0) ? 32768.0 : 32767.0);
}

vrpn_float64 vrpn_XInputGamepad::normalize_trigger(BYTE trigger) const {
	// Filter out low-intensity signals
	if (trigger < XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
		return 0;

	return trigger / 255.0;
}

void vrpn_XInputGamepad::update_vibration() {
	XINPUT_VIBRATION vibration;

	vibration.wLeftMotorSpeed = _motorSpeed[0];
	vibration.wRightMotorSpeed = _motorSpeed[1];

	DWORD rv = XInputSetState(_controllerIndex, &vibration);
	if (rv != ERROR_SUCCESS) {
		char errMsg[256];
		struct timeval now;

		if (rv == ERROR_DEVICE_NOT_CONNECTED)
			sprintf(errMsg, "XInput device %u not connected", _controllerIndex);
		else
			sprintf(errMsg, "XInput device %u returned Windows error code %u",
				_controllerIndex, rv);

		vrpn_gettimeofday(&now, NULL);
		send_text_message(errMsg, now, vrpn_TEXT_ERROR);
		return;
	}
}

void vrpn_XInputGamepad::report(vrpn_uint32 class_of_service) {
	vrpn_Analog::timestamp = _timestamp;
	vrpn_Button::timestamp = _timestamp;

	vrpn_Analog::report(class_of_service);
	vrpn_Button::report_changes();
}

void vrpn_XInputGamepad::report_changes(vrpn_uint32 class_of_service) {
	vrpn_Analog::timestamp = _timestamp;
	vrpn_Button::timestamp = _timestamp;

	vrpn_Analog::report_changes(class_of_service);
	vrpn_Button::report_changes();
}

// Static callback
int VRPN_CALLBACK vrpn_XInputGamepad::handle_last_connection_dropped(void *selfPtr,
	vrpn_HANDLERPARAM data)
{
	vrpn_XInputGamepad *me = static_cast<vrpn_XInputGamepad *>(selfPtr);

	// Kill force feedback if no one is connected
	me->_motorSpeed[0] = 0;
	me->_motorSpeed[1] = 0;
	me->update_vibration();

	return 0;
}


