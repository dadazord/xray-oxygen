#pragma once

class IWallMarkArray;

class IObjectSpaceRender;

class IFontRender;
class IApplicationRender;
class IEnvDescriptorRender;
class IEnvDescriptorMixerRender;
class IFlareRender;
class ILensFlareRender;
class IRainRender;
class IThunderboltRender;
class IEnvironmentRender;
class IStatsRender;
class IRenderDeviceRender;
class IEnvDescriptorRender;
class IThunderboltRender;
class IThunderboltDescRender;
class IRainRender;
class ILensFlareRender;
class IEnvironmentRender;
class IEnvDescriptorMixerRender;
class IStatGraphRender;
class IFlareRender;
class IConsoleRender;
class IUIShader;
class IUISequenceVideoItem;

#define RENDER_FACTORY_INTERFACE(Class) \
virtual I##Class* Create##Class() = 0; \
virtual void Destroy##Class(I##Class *pObject) = 0;
	

class IRenderFactory
{
public:
#ifndef _EDITOR
	RENDER_FACTORY_INTERFACE(UISequenceVideoItem)
	RENDER_FACTORY_INTERFACE(UIShader)
	RENDER_FACTORY_INTERFACE(StatGraphRender)
	RENDER_FACTORY_INTERFACE(ConsoleRender)
	RENDER_FACTORY_INTERFACE(RenderDeviceRender)
	RENDER_FACTORY_INTERFACE(ObjectSpaceRender)
	RENDER_FACTORY_INTERFACE(ApplicationRender)
	RENDER_FACTORY_INTERFACE(WallMarkArray)
	RENDER_FACTORY_INTERFACE(StatsRender)
	RENDER_FACTORY_INTERFACE(EnvironmentRender)
	RENDER_FACTORY_INTERFACE(EnvDescriptorMixerRender)
	RENDER_FACTORY_INTERFACE(EnvDescriptorRender)
	RENDER_FACTORY_INTERFACE(RainRender)
	RENDER_FACTORY_INTERFACE(LensFlareRender)
	RENDER_FACTORY_INTERFACE(ThunderboltRender)
	RENDER_FACTORY_INTERFACE(ThunderboltDescRender)
	RENDER_FACTORY_INTERFACE(FlareRender)
#endif // _EDITOR
	RENDER_FACTORY_INTERFACE(FontRender)
};