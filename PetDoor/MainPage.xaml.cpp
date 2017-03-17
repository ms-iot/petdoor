//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//

#include "pch.h"
#include "MainPage.xaml.h"
#include <ppltasks.h>
#include "MotionSensor.h"

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

#define MOTION_SENSOR_PIN 18
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

		MotionSensor^ motionSensor = ref new MotionSensor(MOTION_SENSOR_PIN, MOTION_SENSOR_TIMER_INTERVAL);

		// Start timer to check for motion
		motionSensor->Start();

		// Add event handler
		motionSensor->MotionDetected +=ref new PetDoor::MotionDetectedEventHandler(this,
				&MainPage::OnMotionDetected);

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


// Called when motion is detected
void MainPage::OnMotionDetected(Object^ sender, Platform::String^ s)
{
	OutputDebugString(s->Data());
}

