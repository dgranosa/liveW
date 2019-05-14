#version 430

#define NB_BARS		128
#define NB_SAMPLES	1
#define SPACE		0.15
#define SIDE_SPACE  0.0

uniform vec2 resolution;
uniform float time;
uniform sampler1D samples;
uniform sampler1D fft;
out vec4 color;

vec3 heatColorMap(float t)
{
    t *= 4.;
    return clamp(vec3(min(t-1.5, 4.5-t), 
                      min(t-0.5, 3.5-t), 
                      min(t+0.5, 2.5-t)), 
                 0., 1.);
}

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
    
    f = texelFetch(fft, bar, 0).r;
    
//    f *= 0.85;
//    f += 0.02;
    
    vec3 c = heatColorMap(f);
    
    
    float bar_f = float(bar)/NB_BARS_F;
    
    //c *= 1.-step(f, uv.y);
    //c *= 1.-step(1.-SPACE*.5, (uv.x-bar_f)*NB_BARS);
    //c *= 1.-step(1.-SPACE*.5, 1.-(uv.x-bar_f)*NB_BARS);
    
    c *= mix(1.,0., clamp((uv.y-f)*resolution.y,0.,1.));
    c *= clamp((min((uv.x-bar_f)*NB_BARS_F, 1.-(uv.x-bar_f)*NB_BARS_F)-SPACE*.5)/NB_BARS_F*resolution.x, 0., 1.);
    color = vec4(0.0);
    if (c != vec3(0.0))
    	color = vec4(c, 1.0);
}
