#include "pch.h"
#include "Servo.h"

using namespace Windows::Devices;
using namespace Windows::Devices::Enumeration;
using namespace Windows::Devices::Gpio;
using namespace Microsoft::IoT::Lightning::Providers;
using namespace concurrency;

namespace PetDoor
{

	// p: Pin connected to the servo
	Servo::Servo(int pin)
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

		auto pwmControllers = create_task(PwmController::GetControllersAsync(LightningPwmProvider::GetPwmProvider())).get();
		auto pwmController = pwmControllers->GetAt(1);
		pwmController->SetDesiredFrequency(SERVO_FREQUENCY);
		_pin = pwmController->OpenPin(pin);
	}

	void Servo::Rotate(double dutyCyclePercentage)
	{
		// Test on Hi-Tec HS-475HB:
		// 0 degrees: ~0.35
		// 180 degrees: ~0.14

		if (dutyCyclePercentage < 0 || dutyCyclePercentage > 0.15)
		{
			throw ref new Platform::Exception(S_FALSE, "Duty cycle percentage must be between 0 and 0.15");
		}

		_pin->SetActiveDutyCyclePercentage(dutyCyclePercentage);
		_pin->Start();
	}
}