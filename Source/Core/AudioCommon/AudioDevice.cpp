
#if defined _WIN32
#include <Mmdeviceapi.h>
#include <Functiondiscoverykeys_devpkey.h>
#endif

#include "AudioCommon/aldlist.h"
#include "AudioDevice.h"
#include "Common/StringUtil.h"
#include "Core/ConfigManager.h"

#define RETURN_ON_LOAD_AUDIO_DEVICES_ERROR(hr) if(FAILED(hr)) { PanicAlert("Error while loading audio device list: %#X", hr); return; }

std::vector<AudioDevice> AudioDevice::devices;
AudioDevice AudioDevice::DEFAULT = AudioDevice(0, "Default", "", "");
bool AudioDevice::loaded = false;

void AudioDevice::LoadDevices() {
	devices.clear();
	std::string backend = SConfig::GetInstance().sBackend;

	AudioDevice::DEFAULT = AudioDevice(0, "Default", "", "");

	if (backend == "XAudio2")
	{
#if defined _WIN32
		HRESULT hr;

		IMMDeviceEnumerator* device_enumerator;
		hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&device_enumerator);
		RETURN_ON_LOAD_AUDIO_DEVICES_ERROR(hr);

		IMMDeviceCollection* device_collection;
		hr = device_enumerator->EnumAudioEndpoints(EDataFlow::eRender, DEVICE_STATE_ACTIVE, &device_collection);
		RETURN_ON_LOAD_AUDIO_DEVICES_ERROR(hr);

		u32 count;
		hr = device_collection->GetCount(&count);
		RETURN_ON_LOAD_AUDIO_DEVICES_ERROR(hr);

		for (u32 i = 0; i < count; i++)
		{
			AudioDevice device;
			device.index = i;

			IMMDevice* device_endpoint;
			hr = device_collection->Item(i, &device_endpoint);
			RETURN_ON_LOAD_AUDIO_DEVICES_ERROR(hr);

			LPWSTR id;
			hr = device_endpoint->GetId(&id);
			RETURN_ON_LOAD_AUDIO_DEVICES_ERROR(hr);
			device.id = TStrToUTF8(id);
			CoTaskMemFree(id);

			IPropertyStore* propStore;
			hr = device_endpoint->OpenPropertyStore(STGM_READ, &propStore);
			RETURN_ON_LOAD_AUDIO_DEVICES_ERROR(hr);

			PROPVARIANT devName;
			PropVariantInit(&devName);
			hr = propStore->GetValue(PKEY_Device_FriendlyName, &devName);
			RETURN_ON_LOAD_AUDIO_DEVICES_ERROR(hr);
			device.name = TStrToUTF8(devName.pwszVal);
			PropVariantClear(&devName);

			PROPVARIANT devPath;
			PropVariantInit(&devPath);
			hr = propStore->GetValue(PROPERTYKEY{ { 0x9c119480, 0xddc2, 0x4954,{ 0xa1, 0x50, 0x5b, 0xd2, 0x40, 0xd4, 0x54, 0xad } }, 1 }, &devPath);
			RETURN_ON_LOAD_AUDIO_DEVICES_ERROR(hr);
			if (devPath.pwszVal != nullptr)
			{
				// seems to not be present in windows 7, which uses XAudio 2.7
				// where the device gets selected by index anyway
				device.path = TStrToUTF8(devPath.pwszVal);
			}
			PropVariantClear(&devPath);

			propStore->Release();
			device_endpoint->Release();

			devices.push_back(device);
		}

		device_enumerator->Release();
		device_collection->Release();
#endif
	}
	else if (backend == "OpenAL")
	{
#if defined HAVE_OPENAL && HAVE_OPENAL
		ALDeviceList pDeviceList;
		if (pDeviceList.GetNumDevices())
		{
			std::string default_id(pDeviceList.GetDeviceName(pDeviceList.GetDefaultDevice()));
			AudioDevice::DEFAULT = AudioDevice(0, "Default (" + default_id + ")", "", default_id);
			for (s32 i = 0; i < pDeviceList.GetNumDevices(); ++i)
			{
				AudioDevice device;
				device.index = i;
				std::string name(pDeviceList.GetDeviceName(i));
				device.id = name;
				device.name = name;
				devices.push_back(device);
			}
		}
		else
		{
			PanicAlertT("OpenAL: can't find sound devices");
		}
#endif
	}

	loaded = true;
}

std::vector<AudioDevice> AudioDevice::GetDevices() {
	if (!loaded) {
		LoadDevices();
	}
	return devices;
}

void AudioDevice::ReloadDevices() {
	LoadDevices();
}

AudioDevice AudioDevice::GetDeviceById(std::string id) {
	auto devices = GetDevices();
	for (AudioDevice device : devices) {
		if (device.id == id) {
			return device;
		}
	}
	// TODO: don't let invalid id's silently fall back to the default device.
	return AudioDevice::DEFAULT;
}

AudioDevice AudioDevice::GetSelectedDevice() {
	return GetDeviceById(SConfig::GetInstance().sAudioDevice);
}
