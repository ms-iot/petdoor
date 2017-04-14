#include "pch.h"
#include "MotionSensor.h"

namespace PetDoor
{

	// p: Pin connected to the motion sensor
	// t: Interval in seconds to check for motion detection
	MotionSensor::MotionSensor(int pin, int timerInterval)
	{
		auto gpio = GpioController::GetDefault();

		if (gpio == nullptr)
		{
			throw ref new Platform::Exception(S_FALSE, "There is no GPIO controller on this device.");
		}

		_timerInterval = timerInterval;

		_pin = gpio->OpenPin(pin);
		_pin->SetDriveMode(GpioPinDriveMode::Input);
	}

	void MotionSensor::Start()
	{
		StartTimer();

		// TODO: Ideally the ValueChanged event should be used but it's not working
		//pin->ValueChanged += ref new TypedEventHandler<GpioPin^, GpioPinValueChangedEventArgs^>(this, &MotionSensor::Pin_ValueChanged);
	}

	// Not used.
	void MotionSensor::StartTimer()
	{
		auto timerDelegate = [this](ThreadPoolTimer^ timer)
		{
			_pinValue = _pin->Read();
			if (_pinValue == GpioPinValue::High)
			{
				// Motion detected, fire the event
				MotionDetected(this, L"Motion detected\n");
			}
			else
			{
				OutputDebugString(L"No motion detected\n");
			}
		};

		if (_pin != nullptr)
		{
			TimeSpan period;
			period.Duration = 10000000 + _timerInterval; // 1 second
			_timer = ThreadPoolTimer::CreatePeriodicTimer(ref new TimerElapsedHandler(timerDelegate), period);
		}
	}

	void MotionSensor::Stop()
	{
		_timer->Cancel();
	}

	void MotionSensor::Pin_ValueChanged(GpioPin ^sender, GpioPinValueChangedEventArgs ^e)
	{
		_pinValue = _pin->Read();
		if (_pinValue == GpioPinValue::High)
		{
			// Motion detected, fire the event
			MotionDetected(this, L"Motion detected\n");
		}
		else
		{
			OutputDebugString(L"No motion detected\n");
		}
	}

}