#pragma once

#include "SoundRender_Target.h"
#include "SoundRender_CoreA.h"

class CSoundRender_TargetA : public CSoundRender_Target
{
	typedef CSoundRender_Target	inherited;

	// OpenAL
	ALuint pSource;
	ALuint pBuffers[sdef_target_count];
	float cache_gain;
	float cache_pitch;

	ALuint buf_block;

private:
	void fill_block(ALuint BufferID);

public:
	CSoundRender_TargetA();
	virtual ~CSoundRender_TargetA();

	void source_changed();

	virtual bool _initialize();
	virtual void _destroy();
	virtual void _restart();

	virtual void start(CSoundRender_Emitter* E);
	virtual void render();
	virtual void rewind();
	virtual void stop();
	virtual void update();
	virtual void fill_parameters();
	virtual	void alAuxInit(ALuint slot);
};