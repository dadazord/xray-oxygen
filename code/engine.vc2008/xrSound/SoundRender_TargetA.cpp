#include "stdafx.h"
#pragma hdrstop

#include "SoundRender_TargetA.h"
#include "SoundRender_Emitter.h"
#include "SoundRender_Source.h"

xr_vector<u8> g_target_temp_data;

CSoundRender_TargetA::CSoundRender_TargetA() :CSoundRender_Target()
{
	cache_gain = 0.f;
	cache_pitch = 1.f;
	pSource = 0;
}

CSoundRender_TargetA::~CSoundRender_TargetA()
{
}
#include <openal\efx.h>

bool CSoundRender_TargetA::_initialize()
{
	inherited::_initialize();
	// initialize buffer
	A_CHK(alGenBuffers(sdef_target_count, pBuffers));
	alGenSources(1, &pSource);
	ALenum error_ = alGetError();

	if (AL_NO_ERROR == error_)
	{
		A_CHK(alSourcei(pSource, AL_LOOPING, AL_FALSE));
		A_CHK(alSourcef(pSource, AL_MIN_GAIN, 0.f));
		A_CHK(alSourcef(pSource, AL_MAX_GAIN, 1.f));
		A_CHK(alSourcef(pSource, AL_GAIN, cache_gain));
		if (strstr(Core.Params, "-snd_speed_ctrl"))
		{
			A_CHK(alSourcef(pSource, AL_PITCH, psSpeedOfSound));
		}
		else
		{
			A_CHK(alSourcef(pSource, AL_PITCH, cache_pitch));
		}
		return true;
	}
	else
	{
		Msg("[OpenA] Can't create source. Error: %s.", alGetString(error_));
		return false;
	}
}

void CSoundRender_TargetA::alAuxInit(ALuint slot)
{
	A_CHK(alSource3i(pSource, AL_AUXILIARY_SEND_FILTER, slot, 0, AL_FILTER_NULL));
}

void	CSoundRender_TargetA::_destroy()
{
	// clean up target
	if (alIsSource(pSource))
		alDeleteSources(1, &pSource);
	A_CHK(alDeleteBuffers(sdef_target_count, pBuffers));
}

void CSoundRender_TargetA::_restart()
{
	_destroy();
	_initialize();
}

void CSoundRender_TargetA::start(CSoundRender_Emitter* E)
{
	inherited::start(E);

	// Calc storage
	buf_block = sdef_target_block * E->source()->m_wformat.nAvgBytesPerSec / 1000;
	g_target_temp_data.resize(buf_block);
}

void	CSoundRender_TargetA::render()
{
	for (u32 buf_idx = 0; buf_idx < sdef_target_count; buf_idx++)
		fill_block(pBuffers[buf_idx]);

	A_CHK(alSourceQueueBuffers(pSource, sdef_target_count, pBuffers));
	A_CHK(alSourcePlay(pSource));

	inherited::render();
}

void	CSoundRender_TargetA::stop()
{
	if (rendering)
	{
		A_CHK(alSourceStop(pSource));
		A_CHK(alSourcei(pSource, AL_BUFFER, 0));
		A_CHK(alSourcei(pSource, AL_SOURCE_RELATIVE, true));
	}
	inherited::stop();
}

void	CSoundRender_TargetA::rewind()
{
	inherited::rewind();

	A_CHK(alSourceStop(pSource));
	A_CHK(alSourcei(pSource, AL_BUFFER, 0));
	for (u32 buf_idx = 0; buf_idx < sdef_target_count; buf_idx++)
		fill_block(pBuffers[buf_idx]);
	A_CHK(alSourceQueueBuffers(pSource, sdef_target_count, pBuffers));
	A_CHK(alSourcePlay(pSource));
}

void	CSoundRender_TargetA::update()
{
	inherited::update();

	ALint processed;
	A_CHK(alGetSourcei(pSource, AL_BUFFERS_PROCESSED, &processed));

	extern const char* DeviceName;
	bool isOALSoft = (!!strstr(DeviceName, "OpenAL Soft"));

	if (processed > 0)
	{
		do
		{
			// kcat: If there's a long enough freeze and the sources underrun, they go to an AL_STOPPED state.
			// That update function will correctly see this and remove/refill/requeue the buffers, but doesn't restart the source
			// (that's in the separate else block that didn't run this time).Because the source remains AL_STOPPED,
			// the next update will still see all the buffers marked as processed and remove / refill / requeue them again.
			// It keeps doing this and never actually restarts the source after an underrun.
			if(isOALSoft)
			{
				ALint state;
				A_CHK(alGetSourcei(pSource, AL_SOURCE_STATE, &state));
				if (state == AL_STOPPED)
				{
					A_CHK(alSourcePlay(pSource));
				}
			}

			ALuint BufferID = 0;
			A_CHK(alSourceUnqueueBuffers(pSource, 1, &BufferID));
			fill_block(BufferID);
			A_CHK(alSourceQueueBuffers(pSource, 1, &BufferID));
			--processed;
		} while (processed > 0);
	}
	else
	{
		// check play status -- if stopped then queue is not being filled fast enough
		ALint state;
		A_CHK(alGetSourcei(pSource, AL_SOURCE_STATE, &state));
		if (state != AL_PLAYING)
		{
			Log("[CSoundRender_TargetA::update()] Queuing underrun detected!");
			A_CHK(alSourcePlay(pSource));
		}
	}
}

void	CSoundRender_TargetA::fill_parameters()
{
	CSoundRender_Emitter* SE = m_pEmitter; VERIFY(SE);

	inherited::fill_parameters();

	// 3D params
	VERIFY2(m_pEmitter, SE->source()->file_name());
	A_CHK(alSourcef(pSource, AL_REFERENCE_DISTANCE, m_pEmitter->p_source.min_distance));

	VERIFY2(m_pEmitter, SE->source()->file_name());
	A_CHK(alSourcef(pSource, AL_MAX_DISTANCE, m_pEmitter->p_source.max_distance));

	VERIFY2(m_pEmitter, SE->source()->file_name());
	A_CHK(alSource3f(pSource, AL_POSITION, m_pEmitter->p_source.position.x, m_pEmitter->p_source.position.y, -m_pEmitter->p_source.position.z));

	VERIFY2(m_pEmitter, SE->source()->file_name());
	A_CHK(alSourcei(pSource, AL_SOURCE_RELATIVE, m_pEmitter->b2D));

	A_CHK(alSourcef(pSource, AL_ROLLOFF_FACTOR, psSoundRolloff));

	VERIFY2(m_pEmitter, SE->source()->file_name());
	float	_gain = m_pEmitter->smooth_volume;
	clamp(_gain, EPS_S, 1.f);

	if (!fsimilar(_gain, cache_gain, 0.01f))
	{
		cache_gain = _gain;
		A_CHK(alSourcef(pSource, AL_GAIN, _gain));
	}

	if (strstr(Core.Params, "-snd_speed_ctrl"))
	{
		A_CHK(alSourcef(pSource, AL_PITCH, psSpeedOfSound));
	}
	else
	{
		float	_pitch = m_pEmitter->p_source.freq;
		clamp(_pitch, EPS_L, 2.f);

		if (!fsimilar(_pitch, cache_pitch))
		{
			cache_pitch = _pitch;
			A_CHK(alSourcef(pSource, AL_PITCH, _pitch));
		}
	}
	VERIFY2(m_pEmitter, SE->source()->file_name());
}

void	CSoundRender_TargetA::fill_block(ALuint BufferID)
{
	R_ASSERT(m_pEmitter);

	m_pEmitter->fill_block(&g_target_temp_data.front(), buf_block);
	ALuint format = (m_pEmitter->source()->m_wformat.nChannels == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
	A_CHK(alBufferData(BufferID, format, &g_target_temp_data.front(), buf_block, m_pEmitter->source()->m_wformat.nSamplesPerSec));
}
void CSoundRender_TargetA::source_changed()
{
	dettach();
	attach();
}