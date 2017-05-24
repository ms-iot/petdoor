#include "Camera.h"


namespace PetDoor 
{
	Camera::Camera()
		: _mediaCapture(nullptr)
		, _isInitialized(false)
		, _isPreviewing(false)
		, _externalCamera(false)
		, _mirroringPreview(false)
		, _displayOrientation(DisplayOrientations::Portrait)
		, _displayRequest(ref new Windows::System::Display::DisplayRequest())
		, RotationKey({ 0xC380465D, 0x2271, 0x428C,{ 0x9B, 0x83, 0xEC, 0xEA, 0x3B, 0x4A, 0x85, 0xC1 } })
		, _captureFolder(nullptr)
	{
	}
	Camera::~Camera() {}
	/// <summary>
	/// Initializes the MediaCapture, registers events, gets camera device information for mirroring and rotating, and starts preview
	/// </summary>
	/// <returns></returns>
	task<void> Camera::InitializeCameraAsync()
	{
		WriteLine("InitializeCameraAsync");

		// Attempt to get the back camera if one is available, but use any camera device if not
		return FindCameraDeviceByPanelAsync(Windows::Devices::Enumeration::Panel::Back)
			.then([this](DeviceInformation^ camera)
		{
			if (camera == nullptr)
			{
				WriteLine("No camera device found!");
				return;
			}
			// Figure out where the camera is located
			if (camera->EnclosureLocation == nullptr || camera->EnclosureLocation->Panel == Windows::Devices::Enumeration::Panel::Unknown)
			{
				// No information on the location of the camera, assume it's an external camera, not integrated on the device
				_externalCamera = true;
			}
			else
			{
				// Camera is fixed on the device
				_externalCamera = false;

				// Only mirror the preview if the camera is on the front panel
				_mirroringPreview = (camera->EnclosureLocation->Panel == Windows::Devices::Enumeration::Panel::Front);
			}

			_mediaCapture = ref new Capture::MediaCapture();

			// Register for a notification when something goes wrong
			_mediaCaptureFailedEventToken =
				_mediaCapture->Failed += ref new Capture::MediaCaptureFailedEventHandler(this, &Camera::MediaCapture_Failed);

			auto settings = ref new Capture::MediaCaptureInitializationSettings();
			settings->VideoDeviceId = camera->Id;

			// Initialize media capture and start the preview
			create_task(_mediaCapture->InitializeAsync(settings))
				.then([this]()
			{
				_isInitialized = true;

				return StartPreviewAsync();
				// Different return types, must do the error checking here since we cannot return and send
				// execeptions back up the chain.
			}).then([this](task<void> previousTask)
			{
				try
				{
					previousTask.get();
				}
				catch (AccessDeniedException^)
				{
					WriteLine("The app was denied access to the camera");
				}
			});
		}).then([this]()
		{
			create_task(StorageLibrary::GetLibraryAsync(KnownLibraryId::Pictures))
				.then([this](StorageLibrary^ picturesLibrary)
			{
				_captureFolder = picturesLibrary->SaveFolder;
				if (_captureFolder == nullptr)
				{
					// In this case fall back to the local app storage since the Pictures Library is not available
					_captureFolder = ApplicationData::Current->LocalFolder;
				}
			});
		});
	}

	/// <summary>
	/// Cleans up the camera resources (after stopping the preview if necessary) and unregisters from MediaCapture events
	/// </summary>
	/// <returns></returns>
	task<void> Camera::CleanupCameraAsync()
	{
		WriteLine("CleanupCameraAsync");

		std::vector<task<void>> taskList;

		if (_isInitialized)
		{
			if (_isPreviewing)
			{
				auto stopPreviewTask = create_task(StopPreviewAsync());
				taskList.push_back(stopPreviewTask);
			}

			_isInitialized = false;
		}

		// When all our tasks complete, clean up MediaCapture
		return when_all(taskList.begin(), taskList.end())
			.then([this]()
		{
			if (_mediaCapture.Get() != nullptr)
			{
				_mediaCapture->Failed -= _mediaCaptureFailedEventToken;
				_mediaCapture = nullptr;
			}
		});
	}

	/// <summary>
	/// Starts the preview and adjusts it for for rotation and mirroring after making a request to keep the screen on and unlocks the UI
	/// </summary>
	/// <returns></returns>
	task<void> Camera::StartPreviewAsync()
	{
		WriteLine("StartPreviewAsync");

		// Prevent the device from sleeping while the preview is running
		_displayRequest->RequestActive();

		// Register to listen for media property changes
		_mediaControlPropChangedEventToken =
			_systemMediaControls->PropertyChanged += ref new TypedEventHandler<SystemMediaTransportControls^, SystemMediaTransportControlsPropertyChangedEventArgs^>(this, &Camera::SystemMediaControls_PropertyChanged);

		// Set the preview source in the UI and mirror it if necessary
		PreviewControl->Source = _mediaCapture.Get();
		PreviewControl->FlowDirection = _mirroringPreview ? Windows::UI::Xaml::FlowDirection::RightToLeft : Windows::UI::Xaml::FlowDirection::LeftToRight;

		// Start the preview
		return create_task(_mediaCapture->StartPreviewAsync())
			.then([this](task<void> previousTask)
		{
			_isPreviewing = true;
			GetPreviewFrameButton->IsEnabled = _isPreviewing;

			// Only need to update the orientation if the camera is mounted on the device
			if (!_externalCamera)
			{
				return SetPreviewRotationAsync();
			}

			// Not external, just return the previous task
			return previousTask;
		});
	}

	/// <summary>
	/// Gets the current orientation of the UI in relation to the device and applies a corrective rotation to the preview
	/// </summary>
	task<void> Camera::SetPreviewRotationAsync()
	{
		// Calculate which way and how far to rotate the preview
		int rotationDegrees = ConvertDisplayOrientationToDegrees(_displayOrientation);

		// The rotation direction needs to be inverted if the preview is being mirrored
		if (_mirroringPreview)
		{
			rotationDegrees = (360 - rotationDegrees) % 360;
		}

		// Add rotation metadata to the preview stream to make sure the aspect ratio / dimensions match when rendering and getting preview frames
		auto props = _mediaCapture->VideoDeviceController->GetMediaStreamProperties(Capture::MediaStreamType::VideoPreview);
		props->Properties->Insert(RotationKey, rotationDegrees);
		return create_task(_mediaCapture->SetEncodingPropertiesAsync(Capture::MediaStreamType::VideoPreview, props, nullptr));
	}

	/// <summary>
	/// Stops the preview and deactivates a display request, to allow the screen to go into power saving modes
	/// </summary>
	/// <returns></returns>
	task<void> Camera::StopPreviewAsync()
	{
		_isPreviewing = false;

		return create_task(_mediaCapture->StopPreviewAsync())
			.then([this]()
		{
			// Use the dispatcher because this method is sometimes called from non-UI threads
			return Dispatcher->RunAsync(Windows::UI::Core::CoreDispatcherPriority::Normal, ref new Windows::UI::Core::DispatchedHandler([this]()
			{
				PreviewControl->Source = nullptr;
				// Allow the device screen to sleep now that the preview is stopped
				_displayRequest->RequestRelease();

				GetPreviewFrameButton->IsEnabled = _isPreviewing;
			}));
		});
	}

	IBuffer^ IBufferFromArray(Array<unsigned char>^ data)
	{
		DataWriter^ dataWriter = ref new DataWriter();
		dataWriter->WriteBytes(data);
		return dataWriter->DetachBuffer();
	}

	IBuffer^ IBufferFromPointer(LPBYTE pbData, DWORD cbData)
	{
		auto byteArray = new ArrayReference<unsigned char>(pbData, cbData);
		return IBufferFromArray(reinterpret_cast<Array<unsigned char>^>(byteArray));
	}

	inline void ThrowIfFailed(HRESULT hr)
	{
		if (FAILED(hr))
		{
			throw Platform::Exception::CreateException(hr);
		}
	}

	std::vector<unsigned char> getVectorFromBuffer(::Windows::Storage::Streams::IBuffer^ buf)
	{
		auto reader = ::Windows::Storage::Streams::DataReader::FromBuffer(buf);

		std::vector<unsigned char> data(reader->UnconsumedBufferLength);

		if (!data.empty())
			reader->ReadBytes(
				::Platform::ArrayReference<unsigned char>(
					&data[0], data.size()));

		return data;
	}

	// Converts an image from a SoftwareBitmap to a Mat file. Mat files are used in OpenCV.
	Mat* SoftwareBitmapToMat(SoftwareBitmap^ image)
	{
		if (!image) return nullptr;
		int height = image->PixelHeight;
		int width = image->PixelWidth;
		int size = image->PixelHeight * image->PixelWidth * 4;


		byte* bytes = new byte[size];
		IBuffer^ buffer = IBufferFromPointer(bytes, size);
		image->CopyToBuffer(buffer);

		std::vector<unsigned char> data = getVectorFromBuffer(buffer);
		memcpy(bytes, data.data(), size);

		Mat* result = new Mat(height, width, CV_8UC4, bytes, cv::Mat::AUTO_STEP);

		return result;
	}

	unsigned char* GetPointerToPixelData(IBuffer^ buffer)
	{
		ComPtr<IBufferByteAccess> bufferByteAccess;
		ComPtr<IInspectable> insp((IInspectable*)buffer);
		ThrowIfFailed(insp.As(&bufferByteAccess));

		unsigned char* pixels = nullptr;
		ThrowIfFailed(bufferByteAccess->Buffer(&pixels));

		return pixels;
	}

	// Converts an image from a Mat file to a Software Bitmap.
	SoftwareBitmap^ MatToSoftwareBitmap(cv::Mat image)
	{
		if (!image.data) return nullptr;

		// Create the WriteableBitmap
		int height = image.rows;
		int width = image.cols;
		int size = height * width * image.step.buf[1];

		SoftwareBitmap^ result = ref new SoftwareBitmap(BitmapPixelFormat::Rgba8, width, height, BitmapAlphaMode::Ignore);

		byte* bytes = new byte[size];
		IBuffer^ buffer = IBufferFromPointer(bytes, size);

		unsigned char* dstPixels = GetPointerToPixelData(buffer);
		memcpy(dstPixels, image.data, image.step.buf[1] * image.cols*image.rows);

		result->CopyFromBuffer(buffer);

		return result;
	}


	// takes an image (inputImg), runs face and body classifiers on it, and stores the results in objectVector and objectVectorBodies, respectively
	void DetectObjects(cv::Mat& inputImg, std::vector<cv::Rect> & objectVector, cv::CascadeClassifier& cat_cascade)
	{
		// create a matrix of unsigned 8-bit int of size rows x cols
		cv::Mat frame_gray = cv::Mat(inputImg.rows, inputImg.cols, CV_8UC4);

		cvtColor(inputImg, frame_gray, CV_RGBA2GRAY);
		cv::equalizeHist(frame_gray, frame_gray);

		// Detect cat faces
		cat_cascade.detectMultiScale(frame_gray, objectVector, 1.1, 5, 0 | CV_HAAR_SCALE_IMAGE, cv::Size(100, 100), cv::Size(300, 300));
	}

	void drawRectOverObjects(Mat& image, std::vector<cv::Rect>& objectVector)
	{
		// Place a red rectangle around all detected objects in image
		for (unsigned int x = 0; x < objectVector.size(); x++)
		{
			cv::rectangle(image, objectVector[x], cv::Scalar(0, 0, 255, 255), 5);
			std::ostringstream catNo;
			catNo << "Cat #" << (x + 1);
			cv::putText(image, catNo.str(), cv::Point(objectVector[x].x, objectVector[x].y - 10), cv::FONT_HERSHEY_SIMPLEX, 0.55, (0, 0, 255), 2);
		}
	}


	// load each image from the file list, detect objects in image and then place rectangles around all detected objects
	/* void processImagesInFolder(cv::String folder, std::vector<cv::String> filenames, cv::CascadeClassifier& cascade)
	{
	glob(folder, filenames);

	for (int i = 0; i < filenames.size(); i++)
	{
	std::vector<cv::Rect> objectVector;
	// load image from folder into image
	cv::Mat image = cv::imread(filenames[i]);

	if (!image.empty()) {

	DetectObjects(image, objectVector, cascade);

	// Place a red rectangle around all detected objects in image
	for (unsigned int x = 0; x < objectVector.size(); x++)
	{
	cv::rectangle(image, objectVector[x], cv::Scalar(0, 0, 255, 255), 5);
	cv::putText(image, "Cat #", cv::Point(objectVector[x].x, objectVector[x].y - 10), cv::FONT_HERSHEY_SIMPLEX, 0.55, (0, 0, 255), 2);
	}

	vector<int> compression_params;
	compression_params.push_back(CV_IMWRITE_JPEG_QUALITY);
	compression_params.push_back(100);

	//StorageFolder ^localFolder = ApplicationData::Current->LocalFolder;
	//auto createFolderTask = create_task(localFolder->CreateFolderAsync("ProcessedPictures"));


	// There were type conflict issues when writing to the Local Storage folder
	// The work around was to hard code the local app data folder path
	// CHANGE LATER
	// WRITE FUNCTION TO SAVE IMAGE TO FILE
	bool writeImage = cv::imwrite(localAppStoragePath + "post_magic" + to_string(i) + ".jpg", image, compression_params);
	}
	}
	}*/

	/// <summary>
	/// Gets the current preview frame as a SoftwareBitmap, displays its properties in a TextBlock, and can optionally display the image
	/// in the UI and/or save it to disk as a jpg
	/// </summary>
	/// <returns></returns>
	task<void> Camera::GetPreviewFrameAsSoftwareBitmapAsync()
	{
		// Get information about the preview
		auto previewProperties = static_cast<MediaProperties::VideoEncodingProperties^>(_mediaCapture->VideoDeviceController->GetMediaStreamProperties(Capture::MediaStreamType::VideoPreview));
		unsigned int videoFrameWidth = previewProperties->Width;
		unsigned int videoFrameHeight = previewProperties->Height;

		// Create the video frame to request a SoftwareBitmap preview frame
		auto videoFrame = ref new VideoFrame(BitmapPixelFormat::Rgba8, videoFrameWidth, videoFrameHeight);

		// Capture the preview frame
		return create_task(_mediaCapture->GetPreviewFrameAsync(videoFrame))
			.then([this](VideoFrame^ currentFrame)
		{
			//MOVE SOMEWHERE ELSE BETTER
			cv::CascadeClassifier cat_cascade;
			//cv::winrt_initContainer(this->cvContainer);

			if (!cat_cascade.load(cat_cascade_name)) {
				printf("Couldnt load cat detector '%s'\n", cat_cascade_name);
				exit(1);
			}

			// Collect the resulting frame
			auto previewFrame = currentFrame->SoftwareBitmap;
			BitmapPixelFormat framepPix = previewFrame->BitmapPixelFormat;
			Mat previewMat = *(SoftwareBitmapToMat(previewFrame));
			//SoftwareBitmap^ previewBitmap = previewFrame;
			std::vector<cv::Rect> objectVector;
			// Show the frame information
			std::wstringstream ss;
			ss << previewFrame->PixelWidth << "x" << previewFrame->PixelHeight << " " << previewFrame->BitmapPixelFormat.ToString()->Data();
			FrameInfoTextBlock->Text = ref new Platform::String(ss.str().c_str());

			// Use openCV to draw rectangles over detected objects
			if (DrawRectanglesCheckBox->IsChecked->Value)
			{

				DetectObjects(previewMat, objectVector, cat_cascade);
				drawRectOverObjects(previewMat, objectVector);
				previewFrame = SoftwareBitmap::Convert(MatToSoftwareBitmap(previewMat), BitmapPixelFormat::Bgra8);
				framepPix = previewFrame->BitmapPixelFormat;
			}

			std::vector<task<void>> taskList;

			// Show the frame (as is, no rotation is being applied)
			if (ShowFrameCheckBox->IsChecked->Value == true)
			{
				// Copy it to a WriteableBitmap to display it to the user
				auto sbSource = ref new Media::Imaging::SoftwareBitmapSource();

				taskList.push_back(create_task(sbSource->SetBitmapAsync(previewFrame))
					.then([this, sbSource]()
				{
					// Display it in the Image control
					PreviewFrameImage->Source = sbSource;
				}));
			}

			// Save the frame (as is, no rotation is being applied)
			if (SaveFrameCheckBox->IsChecked->Value == true)
			{
				taskList.push_back(SaveSoftwareBitmapAsync(previewFrame));
			}

			return when_all(taskList.begin(), taskList.end()).then([currentFrame]() {
				// IClosable.Close projects into CX as operator delete.
				delete currentFrame;
			});
		});
	}

	/// <summary>
	/// Gets the current preview frame as a Direct3DSurface and displays its properties in a TextBlock
	/// </summary>
	/// <returns></returns>
	task<void> Camera::GetPreviewFrameAsD3DSurfaceAsync()
	{
		// Capture the preview frame as a D3D surface
		return create_task(_mediaCapture->GetPreviewFrameAsync())
			.then([this](VideoFrame^ currentFrame)
		{
			std::wstringstream ss;

			// Check that the Direct3DSurface isn't null. It's possible that the device does not support getting the frame
			// as a D3D surface
			if (currentFrame->Direct3DSurface != nullptr)
			{
				// Collect the resulting frame
				auto surface = currentFrame->Direct3DSurface;

				// Show the frame information
				ss << surface->Description.Width << "x" << surface->Description.Height << " " << surface->Description.Format.ToString()->Data();
				FrameInfoTextBlock->Text = ref new Platform::String(ss.str().c_str());
			}
			else // Fall back to software bitmap
			{
				// Collect the resulting frame
				auto previewFrame = currentFrame->SoftwareBitmap;

				// Show the frame information
				ss << previewFrame->PixelWidth << "x" << previewFrame->PixelHeight << " " << previewFrame->BitmapPixelFormat.ToString()->Data();
				FrameInfoTextBlock->Text = ref new Platform::String(ss.str().c_str());
			}

			// Clear the image
			PreviewFrameImage->Source = nullptr;

			// IClosable.Close projects into CX as operator delete.
			delete currentFrame;
		});
	}

	/// <summary>
	/// Saves a SoftwareBitmap to the _captureFolder
	/// </summary>
	/// <param name="bitmap"></param>
	/// <returns></returns>
	task<void> Camera::SaveSoftwareBitmapAsync(SoftwareBitmap^ bitmap)
	{
		return create_task(_captureFolder->CreateFileAsync("PreviewFrame.jpg", CreationCollisionOption::GenerateUniqueName))
			.then([bitmap](StorageFile^ file)
		{
			return create_task(file->OpenAsync(FileAccessMode::ReadWrite));
		}).then([this, bitmap](Streams::IRandomAccessStream^ outputStream)
		{
			return create_task(BitmapEncoder::CreateAsync(BitmapEncoder::JpegEncoderId, outputStream))
				.then([bitmap](BitmapEncoder^ encoder)
			{
				// Grab the data from the SoftwareBitmap
				encoder->SetSoftwareBitmap(bitmap);
				return create_task(encoder->FlushAsync());
			}).then([this, outputStream](task<void> previousTask)
			{
				// IClosable.Close projects into CX as operator delete.
				delete outputStream;
				try
				{
					previousTask.get();
				}
				catch (Platform::Exception^ ex)
				{
					// File I/O errors are reported as exceptions
					WriteException(ex);
				}
			});
		});
	}


	/// <summary>
	/// Applies a basic effect to a Bgra8 SoftwareBitmap in-place
	/// </summary>
	/// <param name="bitmap">SoftwareBitmap that will receive the effect</param>
	void Camera::ApplyGreenFilter(SoftwareBitmap^ bitmap)
	{
		// Effect is hard-coded to operate on BGRA8 format only
		if (bitmap->BitmapPixelFormat == BitmapPixelFormat::Bgra8)
		{
			// In BGRA8 format, each pixel is defined by 4 bytes
			const int BYTES_PER_PIXEL = 4;

			BitmapBuffer^ buffer = bitmap->LockBuffer(BitmapBufferAccessMode::ReadWrite);
			IMemoryBufferReference^ reference = buffer->CreateReference();

			Microsoft::WRL::ComPtr<IMemoryBufferByteAccess> byteAccess;
			if (SUCCEEDED(reinterpret_cast<IUnknown*>(reference)->QueryInterface(IID_PPV_ARGS(&byteAccess))))
			{
				// Get a pointer to the pixel buffer
				byte* data;
				unsigned capacity;
				byteAccess->GetBuffer(&data, &capacity);

				// Get information about the BitmapBuffer
				auto desc = buffer->GetPlaneDescription(0);

				// Iterate over all pixels
				for (int row = 0; row < desc.Height; row++)
				{
					for (int col = 0; col < desc.Width; col++)
					{
						// Index of the current pixel in the buffer (defined by the next 4 bytes, BGRA8)
						auto currPixel = desc.StartIndex + desc.Stride * row + BYTES_PER_PIXEL * col;

						// Read the current pixel information into b,g,r channels (leave out alpha channel)
						auto b = data[currPixel + 0]; // Blue
						auto g = data[currPixel + 1]; // Green
						auto r = data[currPixel + 2]; // Red

													  // Boost the green channel, leave the other two untouched
						data[currPixel + 0] = b;
						data[currPixel + 1] = min(g + 80, 255);
						data[currPixel + 2] = r;
					}
				}
			}
			delete reference;
			delete buffer;
		}
	}

	/// <summary>
	/// Queries the available video capture devices to try and find one mounted on the desired panel
	/// </summary>
	/// <param name="desiredPanel">The panel on the device that the desired camera is mounted on</param>
	/// <returns>A DeviceInformation instance with a reference to the camera mounted on the desired panel if available,
	///          any other camera if not, or null if no camera is available.</returns>
	task<DeviceInformation^> Camera::FindCameraDeviceByPanelAsync(Windows::Devices::Enumeration::Panel panel)
	{
		// Get available devices for capturing pictures
		auto allVideoDevices = DeviceInformation::FindAllAsync(DeviceClass::VideoCapture);

		auto deviceEnumTask = create_task(allVideoDevices);
		return deviceEnumTask.then([panel](DeviceInformationCollection^ devices)
		{
			for (auto cameraDeviceInfo : devices)
			{
				if (cameraDeviceInfo->EnclosureLocation != nullptr && cameraDeviceInfo->EnclosureLocation->Panel == panel)
				{
					return cameraDeviceInfo;
				}
			}

			// Nothing matched, just return the first
			if (devices->Size > 0)
			{
				return devices->GetAt(0);
			}

			// We didn't find any devices, so return a null instance
			DeviceInformation^ camera = nullptr;
			return camera;
		});
	}

	/// <summary>
	/// Writes a given string to the output window
	/// </summary>
	/// <param name="str">String to be written</param>
	void Camera::WriteLine(Platform::String^ str)
	{
		std::wstringstream wStringstream;
		wStringstream << str->Data() << "\n";
		OutputDebugString(wStringstream.str().c_str());
	}

	/// <summary>
	/// Writes a given exception message and hresult to the output window
	/// </summary>
	/// <param name="ex">Exception to be written</param>
	void Camera::WriteException(Platform::Exception^ ex)
	{
		std::wstringstream wStringstream;
		wStringstream << "0x" << ex->HResult << ": " << ex->Message->Data();
		OutputDebugString(wStringstream.str().c_str());
	}

	/// <summary>
	/// Converts the given orientation of the app on the screen to the corresponding rotation in degrees
	/// </summary>
	/// <param name="orientation">The orientation of the app on the screen</param>
	/// <returns>An orientation in degrees</returns>
	int Camera::ConvertDisplayOrientationToDegrees(DisplayOrientations orientation)
	{
		switch (orientation)
		{
		case DisplayOrientations::Portrait:
			return 90;
		case DisplayOrientations::LandscapeFlipped:
			return 180;
		case DisplayOrientations::PortraitFlipped:
			return 270;
		case DisplayOrientations::Landscape:
		default:
			return 0;
		}
	}

	void Camera::Application_Suspending(Object^, Windows::ApplicationModel::SuspendingEventArgs^ e)
	{
		// Handle global application events only if this page is active
		if (Frame->CurrentSourcePageType.Name == Interop::TypeName(Camera::typeid).Name)
		{
			_displayInformation->OrientationChanged -= _displayInformationEventToken;
			auto deferral = e->SuspendingOperation->GetDeferral();
			CleanupCameraAsync()
				.then([this, deferral]()
			{
				deferral->Complete();
			});
		}
	}

	void Camera::Application_Resuming(Object^, Object^)
	{
		// Handle global application events only if this page is active
		if (Frame->CurrentSourcePageType.Name == Interop::TypeName(Camera::typeid).Name)
		{
			// Populate orientation variables with the current state and register for future changes
			_displayOrientation = _displayInformation->CurrentOrientation;
			_displayInformationEventToken =
				_displayInformation->OrientationChanged += ref new TypedEventHandler<DisplayInformation^, Object^>(this, &Camera::DisplayInformation_OrientationChanged);

			InitializeCameraAsync();
		}
	}

	/// <summary>
	/// In the event of the app being minimized this method handles media property change events. If the app receives a mute
	/// notification, it is no longer in the foregroud.
	/// </summary>
	/// <param name="sender"></param>
	/// <param name="args"></param>
	void Camera::SystemMediaControls_PropertyChanged(SystemMediaTransportControls^ sender, SystemMediaTransportControlsPropertyChangedEventArgs^ args)
	{
		Dispatcher->RunAsync(Windows::UI::Core::CoreDispatcherPriority::Normal, ref new Windows::UI::Core::DispatchedHandler([this, sender, args]()
		{
			// Only handle this event if this page is currently being displayed
			if (args->Property == SystemMediaTransportControlsProperty::SoundLevel && Frame->CurrentSourcePageType.Name == Interop::TypeName(Camera::typeid).Name)
			{
				// Check to see if the app is being muted. If so, it is being minimized.
				// Otherwise if it is not initialized, it is being brought into focus.
				if (sender->SoundLevel == SoundLevel::Muted)
				{
					CleanupCameraAsync();
				}
				else if (!_isInitialized)
				{
					InitializeCameraAsync();
				}
			}
		}));
	}

	/// <summary>
	/// This event will fire when the page is rotated
	/// </summary>
	/// <param name="sender">The event source.</param>
	/// <param name="args">The event data.</param>
	void Camera::DisplayInformation_OrientationChanged(DisplayInformation^ sender, Object^)
	{
		_displayOrientation = sender->CurrentOrientation;

		if (_isPreviewing)
		{
			SetPreviewRotationAsync();
		}
	}

	void Camera::GetPreviewFrameButton_Click(Object^, RoutedEventArgs^)
	{
		// If preview is not running, no preview frames can be acquired
		if (!_isPreviewing) return;

		if ((ShowFrameCheckBox->IsChecked->Value == true) || (SaveFrameCheckBox->IsChecked->Value == true))
		{
			GetPreviewFrameAsSoftwareBitmapAsync();
		}
		else
		{
			GetPreviewFrameAsD3DSurfaceAsync();
		}
	}

	void Camera::MediaCapture_Failed(Capture::MediaCapture^, Capture::MediaCaptureFailedEventArgs^ errorEventArgs)
	{
		std::wstringstream ss;
		ss << "MediaCapture_Failed: 0x" << errorEventArgs->Code << ": " << errorEventArgs->Message->Data();
		WriteLine(ref new Platform::String(ss.str().c_str()));

		CleanupCameraAsync()
			.then([this]()
		{
			Dispatcher->RunAsync(Windows::UI::Core::CoreDispatcherPriority::Normal, ref new Windows::UI::Core::DispatchedHandler([this]()
			{
				GetPreviewFrameButton->IsEnabled = _isPreviewing;
			}));
		});
	}

	void Camera::OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs^)
	{
		// Populate orientation variables with the current state and register for future changes
		_displayOrientation = _displayInformation->CurrentOrientation;
		_displayInformationEventToken =
			_displayInformation->OrientationChanged += ref new TypedEventHandler<DisplayInformation^, Object^>(this, &Camera::DisplayInformation_OrientationChanged);

		InitializeCameraAsync();
	}

	void Camera::OnNavigatingFrom(Windows::UI::Xaml::Navigation::NavigatingCancelEventArgs^)
	{
		// Handling of this event is included for completeness, as it will only fire when navigating between pages and this sample only includes one page
		CleanupCameraAsync();

		_displayInformation->OrientationChanged -= _displayInformationEventToken;
	}

}