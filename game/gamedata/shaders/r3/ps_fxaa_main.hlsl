#include "common.h"
#include "ps_fxaa.hlsl"

Texture2D 	s_base0;
uniform float4		screen_res;	

struct	v2p 
{
	float2 	Tex0	: TEXCOORD0;
	float4 	HPos	: SV_Position;	 
};

float4 main (  v2p I ) : SV_Target
{
	float2 rcpFrame = float2(1.0/screen_res.x, 1.0/screen_res.y);
	FxaaTex tex = { smp_rtlinear, s_base0};

	return FxaaPixelShader(I.Tex0,
            FxaaFloat4(0.0f, 0.0f, 0.0f, 0.0f),		// FxaaFloat4 fxaaConsolePosPos,
            tex,							// FxaaTex tex,
            tex,							// FxaaTex fxaaConsole360TexExpBiasNegOne,
            tex,							// FxaaTex fxaaConsole360TexExpBiasNegTwo,
            rcpFrame,							// FxaaFloat2 fxaaQualityRcpFrame,
            FxaaFloat4(0.0f, 0.0f, 0.0f, 0.0f),		// FxaaFloat4 fxaaConsoleRcpFrameOpt,
            FxaaFloat4(0.0f, 0.0f, 0.0f, 0.0f),		// FxaaFloat4 fxaaConsoleRcpFrameOpt2,
            FxaaFloat4(0.0f, 0.0f, 0.0f, 0.0f),		// FxaaFloat4 fxaaConsole360RcpFrameOpt2,
            0.35f,									// FxaaFloat fxaaQualitySubpix,
            0.166f,									// FxaaFloat fxaaQualityEdgeThreshold,
            0.0f,								// FxaaFloat fxaaQualityEdgeThresholdMin,
            0.0f,									// FxaaFloat fxaaConsoleEdgeSharpness,
            0.0f,									// FxaaFloat fxaaConsoleEdgeThreshold,
            0.0f,									// FxaaFloat fxaaConsoleEdgeThresholdMin,
            FxaaFloat4(0.0f, 0.0f, 0.0f, 0.0f)		// FxaaFloat fxaaConsole360ConstDir,
            );
	
}
