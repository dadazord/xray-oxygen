#include "common_iostructs.h"
#include "ogse_config.h"
#include "ogse_functions.h"
#include "shared\common.h"
#include "shared\wmark.h"

uniform float4 screen_res;
uniform float4 c_sunshafts;		// x - exposure, y - density
uniform float3 L_sun_dir_e;

Texture2D jitter0;
Texture2D jitter1;
Texture2D s_noise;
Texture2D s_sun_shafts; // current sunshafts texture
sampler smp_jitter;

float3 blendSoftLight(float3 a, float3 b)
{
	float3 c = 2 * a * b * ( 1 + a * (  1 - b ) );
	float3 a_sqrt = sqrt( a );
	float3 d = ( a  +  b * (a_sqrt - a )) * 2 - a_sqrt;
	return( b < 0.5 )? c : d;
};

float4 main(p_screen I) : SV_Target
{	
	// dist to the sun
	float sun_dist = FARPLANE / (sqrt(1 - L_sun_dir_w.y * L_sun_dir_w.y));
	// sun pos
	float3 sun_pos_world = sun_dist*L_sun_dir_w + eye_position;
	float4 sun_pos_projected = mul(m_VP, float4(sun_pos_world, 1));
	float4 sun_pos_screen = proj_to_screen(sun_pos_projected) / sun_pos_projected.w;
	// sun-pixel vector
	float2 sun_vec_screen = normalize(sun_pos_screen.xy - I.tc0.xy);
	// smooth shafts
	float4 cSunShafts = s_sun_shafts.Sample(smp_rtlinear, I.tc0.xy);
#ifdef SUNSHAFTS_QUALITY
#	if SUNSHAFTS_QUALITY>1
	cSunShafts *= 0.5;
	cSunShafts += s_sun_shafts.Sample(smp_rtlinear, I.tc0.xy + sun_vec_screen.yx * screen_res.zw) * 0.25;
	cSunShafts += s_sun_shafts.Sample(smp_rtlinear, I.tc0.xy - sun_vec_screen.yx * screen_res.zw) * 0.25;
#	endif
#endif
	float3 img = s_image.Sample(smp_rtlinear, I.tc0.xy).xyz;

	float dust_size = 8/SS_DUST_SIZE;
	float3 jit;
	float2 jtc = I.tc0.xy;
	float2 sun_dir_e = L_sun_dir_e.xy;
	sun_dir_e /= sin(ogse_c_screen.y);
	sun_dir_e *= ogse_c_screen.x;
	
	jtc.x += sun_dir_e.x;
	jtc.y -= sun_dir_e.y;
	jtc.x = (jtc.x > 1.0)?(jtc.x-1.0):jtc.x;
	jtc.y = (jtc.y < 0.0)?(1.0-jtc.y):jtc.y;
	
	jit.x = jitter0.Sample(smp_jitter, float2(jtc.x, jtc.y + timers.x*0.01*SS_DUST_SPEED) * dust_size).x;
	jit.y = jitter1.Sample(smp_jitter, float2(jtc.x + timers.x*0.01*SS_DUST_SPEED, jtc.y) * dust_size).y;
	jit.z = s_noise.Sample(smp_linear, jtc).x;
	jit.z = saturate(jit.z + SS_DUST_DENSITY - 1);
	
	float dust = saturate(jit.x*jit.y*jit.z);
	float len = length(dust);
	dust *= SS_DUST_INTENSITY;
	
#ifndef USE_MSAA
	float dep = s_position.Sample(smp_nofilter, I.tc0.xy).z;
#else
	float dep = s_position.Load(int3(I.tc0.xy * pos_decompression_params2.xy, 0), 0).z;
#endif
	dust = lerp(0, dust, (1 - saturate(dep * 0.2)) * (1 - saturate(is_sky(dep))));
	dust += 1.0;
	
	float3 Color =  img + cSunShafts.xyz * L_sun_color.xyz * ( 1 - img ) * SS_INTENSITY * c_sunshafts.x * dust;
	//Color = blendSoftLight(Color, L_sun_color.xyz * cSunShafts.w * SS_BLEND_FACTOR * 0.5 + 0.5);
	return float4(Color.x, Color.y, Color.z, 1.0);
}