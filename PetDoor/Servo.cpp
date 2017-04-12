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
			throw ref new Platform::Exception(S_FALSE, "Lightning is not enabled in this device.");
		}

		auto gpio = GpioController::GetDefault();

		if (gpio == nullptr)
		{
			throw ref new Platform::Exception(S_FALSE, "There is no GPIO controller on this device.");
		}

		auto pwmControllers = create_task(PwmController::GetControllersAsync(LightningPwmProvider::GetPwmProvider())).get();
		auto pwmController = pwmControllers->GetAt(1);
		_pin = pwmController->OpenPin(pin);
	}

	void Servo::Rotate(int degrees)
	{
		// TODO: Convert degrees to duty cycle
		//_pin->SetActiveDutyCyclePercentage();
		//_pin->Start();
	}
}