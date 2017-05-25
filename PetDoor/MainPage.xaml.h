//
// MainPage.xaml.h
// Declaration of the MainPage class.
//

#pragma once

#include "MainPage.g.h"
#include "MotionSensor.h"
#include "Servo.h"

using namespace Windows::Devices::Gpio;
using namespace Concurrency;

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

		// Receive notifications about rotation of the device and UI and apply any necessary rotation to the preview stream and UI controls  
		Windows::Graphics::Display::DisplayInformation^ _displayInformation;
		Windows::Graphics::Display::DisplayOrientations _displayOrientation;

		void Run();
		void OnIndoorMotionDetected(Object^ sender, Platform::String^ s);
		void OnOutdoorMotionDetected(Object^ sender, Platform::String^ s);
		//void InitLED();
		void InitMotionSensors();
		void InitServos();
		void OpenDoor(int milliseconds);

		// Event handlers
		void Application_Suspending(Object^ sender, Windows::ApplicationModel::SuspendingEventArgs^ e);
		void Application_Resuming(Object^ sender, Object^ args);
		void SystemMediaControls_PropertyChanged(Windows::Media::SystemMediaTransportControls^ sender, Windows::Media::SystemMediaTransportControlsPropertyChangedEventArgs^ args);

		// Event Tokens
		Windows::Foundation::EventRegistrationToken _displayInformationEventToken;
		void DisplayInformation_OrientationChanged(Windows::Graphics::Display::DisplayInformation^ sender, Object^ args);

	protected:
		virtual void OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e) override;
		virtual void OnNavigatingFrom(Windows::UI::Xaml::Navigation::NavigatingCancelEventArgs^ e) override;
	};
}
