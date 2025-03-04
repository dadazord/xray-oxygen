/*
 * Copyright (c) 2005, Creative Labs Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided
 * that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and
 * 	     the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions
 * 	     and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of Creative Labs Inc. nor the names of its contributors may be used to endorse or
 * 	     promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include "stdafx.h"
#include "OpenALDeviceList.h"

#pragma warning(disable: 4995 4805)
#include <objbase.h>
#pragma warning(default: 4995)

const char* DeviceName;

ALDeviceList::ALDeviceList()
{
	snd_device_id = u32(-1);
	Enumerate();
}

/*
 * Exit call
 */
ALDeviceList::~ALDeviceList()
{
	for (int i = 0; snd_devices_token[i].name; ++i)
	{
		xr_free(snd_devices_token[i].name);
	}
	xr_free(snd_devices_token);
	snd_devices_token = nullptr;
}

void ALDeviceList::Enumerate()
{
	char *devices;
	int major, minor, index;
	const char* actualDeviceName;

	Msg("SOUND: OpenAL: enumerate devices...");
	// have a set of vectors storing the device list, selection status, spec version #, and XRAM support status
	// -- empty all the lists and reserve space for 10 devices
	m_devices.clear();

	CoUninitialize();
	// grab function pointers for 1.0-API functions, and if successful proceed to enumerate all devices
	if (alcIsExtensionPresent(nullptr, "ALC_ENUMERATION_EXT"))
	{
		Msg("SOUND: OpenAL: EnumerationExtension Present");

		devices = (char *)alcGetString(nullptr, ALC_DEVICE_SPECIFIER);
		Msg("devices %s", devices);
		xr_strcpy(m_defaultDeviceName, (char *)alcGetString(nullptr, ALC_DEFAULT_DEVICE_SPECIFIER));
		Msg("SOUND: OpenAL: system  default SndDevice name is %s", m_defaultDeviceName);

		// ManowaR
		// "Generic Hardware" device on software AC'97 codecs introduce
		// high CPU usage ( up to 30% ) as a consequence - freezes, FPS drop
		// So if default device is "Generic Hardware" which maps to DirectSound3D interface
		// We re-assign it to "Generic Software" to get use of old good DirectSound interface
		// This makes 3D-sound processing unusable on cheap AC'97 codecs
		// Also we assume that if "Generic Hardware" exists, than "Generic Software" is also exists
		// Maybe wrong

		if (0 == stricmp(m_defaultDeviceName, AL_GENERIC_HARDWARE))
		{
			xr_strcpy(m_defaultDeviceName, AL_GENERIC_SOFTWARE);
			Msg("SOUND: OpenAL: default SndDevice name set to %s", m_defaultDeviceName);
		}

		index = 0;
		// go through device list (each device terminated with a single nullptr, list terminated with double nullptr)
		while (*devices != 0)
		{
			ALCdevice *device = alcOpenDevice(devices);
			if (device)
			{
				ALCcontext *context = alcCreateContext(device, nullptr);
				if (context)
				{
					alcMakeContextCurrent(context);
					// if new actual device name isn't already in the list, then add it...
					actualDeviceName = alcGetString(device, ALC_DEVICE_SPECIFIER);

					if ((actualDeviceName != nullptr) && xr_strlen(actualDeviceName) > 0)
					{
						alcGetIntegerv(device, ALC_MAJOR_VERSION, sizeof(int), &major);
						alcGetIntegerv(device, ALC_MINOR_VERSION, sizeof(int), &minor);
						m_devices.push_back(ALDeviceDesc(actualDeviceName, minor, major));

						m_devices.back().props.efx = alcIsExtensionPresent(alcGetContextsDevice(alcGetCurrentContext()), "ALC_EXT_EFX");
						m_devices.back().props.xram = alcIsExtensionPresent(alcGetContextsDevice(alcGetCurrentContext()), "EAX_RAM");

						Msg("[OpenAL] device: %s, EFX Support: %s", actualDeviceName, m_devices.back().props.efx ? "yes" : "no");

						m_devices.back().props.eax_unwanted = ((0 == xr_strcmp(actualDeviceName, AL_GENERIC_HARDWARE)) ||
							(0 == xr_strcmp(actualDeviceName, AL_GENERIC_SOFTWARE)));
						++index;
					}
					alcDestroyContext(context);
				}
				else
					Msg("SOUND: OpenAL: cant create context for %s", device);

				alcCloseDevice(device);
			}
			else
				Msg("SOUND: OpenAL: cant open device %s", devices);

			devices += xr_strlen(devices) + 1;
		}
	}
	else
		Msg("SOUND: OpenAL: EnumerationExtension NOT Present");

	//make token
	u32 _cnt = GetNumDevices();
	snd_devices_token = xr_alloc<xr_token>(_cnt + 1);
	snd_devices_token[_cnt].id = -1;
	snd_devices_token[_cnt].name = nullptr;

	for (u32 i = 0; i < _cnt; ++i)
	{
		snd_devices_token[i].id = i;
		snd_devices_token[i].name = xr_strdup(m_devices[i].name);
	}
	//--

	if (0 != GetNumDevices())
		Msg("SOUND: OpenAL: All available devices:");

	int majorVersion, minorVersion;

	for (u32 j = 0; j < GetNumDevices(); ++j)
	{
		GetDeviceVersion(j, &majorVersion, &minorVersion);
	}
	if (!strstr(GetCommandLine(), "-editor"))
		CoInitializeEx(nullptr, COINIT_MULTITHREADED);
}

const char* ALDeviceList::GetDeviceName(u32 index)
{
	return snd_devices_token[index].name;
}

void ALDeviceList::SelectBestDevice()
{
	int best_majorVersion = -1;
	int best_minorVersion = -1;
	int majorVersion;
	int minorVersion;

	if (snd_device_id == u32(-1))
	{
		//select best
		u32 new_device_id = snd_device_id;
		for (u32 i = 0; i < GetNumDevices(); ++i)
		{
			if (stricmp(m_defaultDeviceName, GetDeviceName(i)) != 0)
				continue;

			GetDeviceVersion(i, &majorVersion, &minorVersion);
			if ((majorVersion > best_majorVersion) ||
				(majorVersion == best_majorVersion && minorVersion > best_minorVersion))
			{
				best_majorVersion = majorVersion;
				best_minorVersion = minorVersion;
				new_device_id = i;
			}
		}

		if (new_device_id == u32(-1))
		{
			R_ASSERT(GetNumDevices() != 0);
			new_device_id = 0; //first
		}

		snd_device_id = new_device_id;
	}
	if (GetNumDevices())
	{
		DeviceName = GetDeviceName(snd_device_id);
		Msg("[SOUND]: Selected device is [%s]", DeviceName);
	}
	else Msg("[SOUND]: Can't select device. List empty");
}

/*
 * Returns the major and minor version numbers for a device at a specified index in the complete list
 */
void ALDeviceList::GetDeviceVersion(u32 index, int *major, int *minor)
{
	*major = m_devices[index].major_ver;
	*minor = m_devices[index].minor_ver;
}