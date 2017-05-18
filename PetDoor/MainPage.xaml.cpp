//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//

#include "pch.h"
#include "MainPage.xaml.h"
#include <ppltasks.h>
#include "MotionSensor.h"
#include "Servo.h"

using namespace PetDoor;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Microsoft::IoT::Lightning::Providers;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;
using namespace concurrency;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::UI::Core;
using namespace Windows::System::Threading;
using namespace Windows::Devices;

// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409


#define LEFT_SERVO 7 //White
#define RIGHT_SERVO 1 //Gray
#define MOTION_SENSOR_PIN_OUTDOOR 6 // Orange
#define MOTION_SENSOR_PIN_INDOOR 8 //Yellow
#define MOTION_SENSOR_TIMER_INTERVAL 1 // In seconds

MainPage::MainPage()
{
	InitializeComponent();
	Run();
}

void MainPage::Run()
{
	auto workItem = ref new WorkItemHandler(
		[this](IAsyncAction^ workItem)
	{
		if (LightningProvider::IsLightningEnabled)
		{
			LowLevelDevicesController::DefaultProvider = LightningProvider::GetAggregateProvider();
		}
		else
		{
			throw ref new Platform::Exception(E_FAIL, "Lightning is not enabled in this device.");
		}

		auto gpio = GpioController::GetDefault();

		if (gpio == nullptr)
		{
			throw ref new Platform::Exception(S_FALSE, "There is no GPIO controller on this device.");
		}

		//InitLED();
		InitServos();
		InitMotionSensors();

		// Note: Servo usage
		//Servo^ leftServo = ref new Servo(LEFT_SERVO);
		//Servo^ rightServo = ref new Servo(RIGHT_SERVO);
		//leftServo->Rotate(...);
		//rightServo->Rotate(...);

		// Note: Use code below when UI thread needs to be accessed (e.g. update a text box)
		//CoreApplication::MainView->CoreWindow->Dispatcher->RunAsync(
		//	CoreDispatcherPriority::High,
		//	ref new DispatchedHandler([this]()
		//{
		//}));
	});

	// Running motion sensor code in new thread (not UI thread)
	auto asyncAction = ThreadPool::RunAsync(workItem);
}

void MainPage::InitMotionSensors()
{
	//motionSensorOutdoor = ref new MotionSensor(MOTION_SENSOR_PIN_OUTDOOR, MOTION_SENSOR_TIMER_INTERVAL);
	motionSensorIndoor = ref new MotionSensor(MOTION_SENSOR_PIN_INDOOR, MOTION_SENSOR_TIMER_INTERVAL);

	// Start timers to check for motion
	//motionSensorOutdoor->Start();
	motionSensorIndoor->Start();

	// Add event handlers
	//motionSensorOutdoor->MotionDetected += ref new PetDoor::MotionDetectedEventHandler(this,
	//&MainPage::OnOutdoorMotionDetected);
	motionSensorIndoor->MotionDetected += ref new PetDoor::MotionDetectedEventHandler(this,
		&MainPage::OnIndoorMotionDetected);
}

void MainPage::InitServos()
{
	rightServo = ref new Servo(RIGHT_SERVO);
	leftServo = ref new Servo(LEFT_SERVO);
}

////void MainPage::InitLED()
////{
////	auto gpio = GpioController::GetDefault();
////
////	if (gpio == nullptr)
////	{
////		throw ref new Platform::Exception(E_FAIL, "There is no GPIO controller on this device.");
////	}
////
////	ledPin = gpio->OpenPin(LED);
////	ledPin->Write(GpioPinValue::High);
////	ledPin->SetDriveMode(GpioPinDriveMode::Output);
////}

// Called when motion is detected outdoors
void MainPage::OnOutdoorMotionDetected(Object^ sender, Platform::String^ s)
{
	OutputDebugString(L"Outdoor motion detected\n");
}

void MainPage::OpenDoor(int milliseconds = 1000)
{
	rightServo->Rotate(0.1225); // Open
	leftServo->Rotate(0.0288); // Open
	Sleep(milliseconds);
	leftServo->Rotate(0.0755); // Closed
	rightServo->Rotate(0.0785); //Closed
}

// Called when motion is detected indoors
void MainPage::OnIndoorMotionDetected(Object^ sender, Platform::String^ s)
{
	OpenDoor();
	OutputDebugString(L"Indoor motion detected\n");
}



