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

// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

#define MOTION_SENSOR_PIN_OUTDOOR 18
#define MOTION_SENSOR_PIN_INDOOR 23

#define LEFT_SERVO 24
#define RIGHT_SERVO 25

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
		InitMotionSensors();
		
		// TODO: Servo usage
		//Servo^ leftServo = ref new Servo(LEFT_SERVO);
		//Servo^ rightServo = ref new Servo(RIGHT_SERVO);
		//leftServo->Rotate(...);
		//rightServo->Rotate(...);


		// TODO: Use code below when UI thread needs to be accessed (e.g. update a text box)
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
	MotionSensor^ motionSensorOutdoor = ref new MotionSensor(MOTION_SENSOR_PIN_OUTDOOR, MOTION_SENSOR_TIMER_INTERVAL);
	MotionSensor^ motionSensorIndoor = ref new MotionSensor(MOTION_SENSOR_PIN_INDOOR, MOTION_SENSOR_TIMER_INTERVAL);

	// Start timers to check for motion
	motionSensorOutdoor->Start();
	motionSensorIndoor->Start();

	// Add event handlers
	motionSensorOutdoor->MotionDetected += ref new PetDoor::MotionDetectedEventHandler(this,
		&MainPage::OnOutdoorMotionDetected);
	motionSensorIndoor->MotionDetected += ref new PetDoor::MotionDetectedEventHandler(this,
		&MainPage::OnIndoorMotionDetected);
}

// Called when motion is detected outdoors
void MainPage::OnOutdoorMotionDetected(Object^ sender, Platform::String^ s)
{
	OutputDebugString(L"Outdoor motion detected\n");
}


// Called when motion is detected indoors
void MainPage::OnIndoorMotionDetected(Object^ sender, Platform::String^ s)
{
	OutputDebugString(L"Indoor motion detected\n");
}



