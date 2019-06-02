#version 430

#define NB_BARS         100
#define NB_SAMPLES      1
#define SPACE           0.05
#define SIDE_SPACE      0.10
#define HEIGHT          0.50

#define IMAGE_X         0.10
#define IMAGE_Y         0.33
#define IMAGE_WIDTH     0.10
#define IMAGE_HEIGHT    0.15

uniform vec2 resolution;
uniform float time;
uniform sampler1D samples;
uniform sampler1D fft;
uniform float position;
uniform sampler2D albumArt;
out vec4 color;

void main() {
    
    vec2 uv = gl_FragCoord.xy / resolution.xy;

    vec2 uuv = uv;

    uv.x = (uv.x - SIDE_SPACE) / (1.0 - 2.0 *SIDE_SPACE);
    
    if(uv.x < 0.0 || uv.x > 1.0)
    {
    	color = vec4(0.);
        return;
    }

	vec2 texSize = textureSize(albumArt, 0);
    if (texSize.x > 1
     && IMAGE_X <= uuv.x && uuv.x <= IMAGE_X + IMAGE_WIDTH
     && IMAGE_Y <= uuv.y && uuv.y <= IMAGE_Y + IMAGE_HEIGHT) {

		vec2 p = (uuv - vec2(IMAGE_X, IMAGE_Y)) / vec2(IMAGE_WIDTH, IMAGE_HEIGHT);

        float xx = 1.0;

        if (texture(albumArt, vec2(0.0)).xyz == vec3(0.0)) {
            xx = 0.75;
        }

        p.y = p.y * xx + (1.0 - xx) / 2;
		if (texSize.x == texSize.y)
			color = texture(albumArt, p);
		else {
			float r = texSize.y / texSize.x * xx;
			color = texture(albumArt, vec2(p.x * r + (1.0 - r) / 2.0, p.y));
		}
		return;
	
    }

    if (uv.y < HEIGHT - 0.005 && uv.y > HEIGHT - 0.015) {
        if (uv.x < position)
            color = vec4(1.0, 1.0, 1.0, 1.0);
        else
            color = vec4(1.0, 1.0, 1.0, 0.2);
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
    
    f *= 0.3;
	//f -= 0.0;

	if (f <= 0.001)
		f = 0.001;
    
	float bar_f = float(bar) / NB_BARS_F;

	color = vec4(1.0);
	color *= clamp((min((uv.x - bar_f) * NB_BARS_F, 1.0 - (uv.x - bar_f) * NB_BARS_F) - SPACE * 0.5) / NB_BARS_F * resolution.x, 0.0, 1.0);

	if (uv.y < HEIGHT || uv.y > HEIGHT + f)
		color = vec4(0.0);
}
