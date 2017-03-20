#include "pch.h"
#include "MotionSensor.h"

// p: Pin connected to the motion sensor
// t: Interval in seconds to check for motion detection
PetDoor::MotionSensor::MotionSensor(int p, int t)
{
	auto gpio = GpioController::GetDefault();

	if (gpio == nullptr)
	{
		throw ref new Platform::Exception(S_FALSE, "There is no GPIO controller on this device.");
	}

	timerInterval = t; 

	pin = gpio->OpenPin(p);
	pin->SetDriveMode(GpioPinDriveMode::Input);
}

void PetDoor::MotionSensor::Start()
{
	auto timerDelegate = [this](ThreadPoolTimer^ timer)
	{
		if (pinValue == GpioPinValue::High)
		{
			// Motion detected, fire the event
			MotionDetected(this, L"Motion detected\n");
		}
		else
		{
			OutputDebugString(L"No motion detected\n");
		}
	};

	if (pin != nullptr)
	{
		TimeSpan period;
		period.Duration = 10000000 + timerInterval; // 1 second
		timer = ThreadPoolTimer::CreatePeriodicTimer(ref new TimerElapsedHandler(timerDelegate), period);
	}
}

void PetDoor::MotionSensor::Stop()
{
	timer->Cancel();
}