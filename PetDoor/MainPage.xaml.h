//
// MainPage.xaml.h
// Declaration of the MainPage class.
//

#pragma once

#include "MainPage.g.h"
#include "MotionSensor.h"
#include "Servo.h"

using namespace Windows::Devices::Gpio;

namespace PetDoor
{
	/// <summary>
	/// An empty page that can be used on its own or navigated to within a Frame.
	/// </summary>
	public ref class MainPage sealed
	{
	public:
		MainPage();
	private:
		GpioPin^ ledPin;
		MotionSensor^ motionSensorIndoor;
		MotionSensor^ motionSensorOutdoor;
		Servo^ leftServo;
		Servo^ rightServo;
		void Run();
		void OnIndoorMotionDetected(Object^ sender, Platform::String^ s);
		void OnOutdoorMotionDetected(Object^ sender, Platform::String^ s);
		//void InitLED();
		void InitMotionSensors();
		void InitServos();
		void OpenDoor(int milliseconds);
	};
}
