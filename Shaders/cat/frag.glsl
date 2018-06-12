#version 430

#define NB_BARS		64
#define NB_SAMPLES	8
#define SPACE		0.05
#define SIDE_SPACE  0.10
#define HEIGHT      0.5

uniform vec2 resolution;
uniform float time;
uniform sampler1D samples;
uniform sampler1D fft;
out vec4 color;

void main() {
    
    vec2 uv = gl_FragCoord.xy/resolution.xy;
    
    uv.x = (uv.x-SIDE_SPACE)/(1.-2.*SIDE_SPACE);
    
    if(uv.x<0. || uv.x > 1.)
    {
    	color = vec4(0.);
        return;
    }
    
    float NB_BARS_F = float(NB_BARS);
    int bar = int(floor(uv.x * NB_BARS_F));
    
    float f = 0.;
    f = 0.;
    
    for(int t=0; t<NB_SAMPLES; t++)
    {
    	f += texelFetch(fft, bar*NB_SAMPLES+t, 0).r;
    }
    f /= float(NB_SAMPLES);
    
    f *= 0.6;
	f -= 0.1;

	if (f <= 0.001)
		f = 0.001;
    
	float bar_f = float(bar) / NB_BARS_F;

	color = vec4(1.0);
	color *= clamp((min((uv.x - bar_f) * NB_BARS_F, 1.0 - (uv.x - bar_f) * NB_BARS_F) - SPACE * 0.5) / NB_BARS_F * resolution.x, 0.0, 1.0);

	if (uv.y < HEIGHT || uv.y > HEIGHT + f)
		color = vec4(0.0);
}
