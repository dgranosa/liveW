#version 430

#define SPACE 0.001
#define SPACE_WIDTH 0.0

#define HEIGHT 0.2
#define WIDTH 0.8

#define START 0.2
#define TRANS 0.8

uniform vec2 resolution;
uniform float time;
uniform sampler1D samples;
uniform sampler1D fft;
out vec4 color;

void main()
{
	vec2 uv = gl_FragCoord.xy / resolution.xy;

	float hlf = WIDTH / 2.0;
	
	float left = (1.0 - WIDTH) / 2.0;
	float right = left + WIDTH;

	if (uv.x < left || uv.x > right) {
		color = vec4(0.0);
		return;
	}

	float b;

	if (uv.x < left + hlf)
		b = 1.0 - (uv.x - left) / hlf;
	else
		b = (uv.x - left - hlf) / hlf;

	float f = texture(fft, b).x;
	float t = TRANS - abs(uv.x - hlf - left);

	float top = HEIGHT + f;
	float bottom = HEIGHT - f;

	if (mod(uv.x, SPACE) <= SPACE_WIDTH) {
		color = vec4(0.0);
		return;
	}

	if (left <= uv.x && uv.x <= right && bottom <= uv.y && uv.y <= top)
		color = vec4(vec3(0.1), t);
	else
		color = vec4(0.0);

	return;
}
