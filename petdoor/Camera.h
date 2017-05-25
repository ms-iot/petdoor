#pragma once

#include "pch.h"
#include <array>
#include <iostream>

#include <MemoryBuffer.h>   // IMemoryBufferByteAccess
#include <opencv2\imgproc\types_c.h>
#include <opencv2\imgcodecs\imgcodecs.hpp>
#include <opencv2\core\core.hpp>
#include <opencv2\imgproc\imgproc.hpp>
#include <opencv2\objdetect.hpp>
#include <opencv2\highgui.hpp>
#include <Robuffer.h>


using namespace PetDoor;
using namespace Platform;
using namespace Concurrency;
using namespace Windows::Devices::Enumeration;
using namespace Windows::Devices::Sensors;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Graphics::Display;
using namespace Windows::Graphics::Imaging;
using namespace Windows::Media;
using namespace Windows::Storage;
using namespace Windows::Storage::Streams;
using namespace Windows::System::Threading;
using namespace cv;
using namespace std;

using namespace Microsoft::WRL;

namespace PetDoor 
{
	public ref class Camera sealed 
	{
	private:
		

	

	public:
		Camera();
		virtual ~Camera();
		/*MainPage();
		virtual ~MainPage();*/

	};
}