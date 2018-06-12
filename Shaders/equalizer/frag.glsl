#version 430
uniform vec2 resolution;
uniform float time;
uniform sampler1D samples;
uniform sampler1D fft;
out vec4 color;

#define circleX 0.45
#define circleY 0.45

vec2 UNIT_VECTOR;
vec2 UNIT_VECTOR_ASPECT;

float drawEq();
float drawFlare(float intensity);
float drawDistortedRing(float radius0, float sharpness);
float drawRing(float radius0, float sharpness);

void main()
{
	UNIT_VECTOR = vec2(gl_FragCoord.x/resolution.x, gl_FragCoord.y / resolution.y);
	UNIT_VECTOR_ASPECT = vec2(UNIT_VECTOR.x * resolution.x/resolution.y - (resolution.x - resolution.y)/resolution.x, UNIT_VECTOR.y);

	vec4 baseColor = vec4(0.1, 0.5 * sin(UNIT_VECTOR.x) * 0.3, 0.55 + 0.25 * sin(time * 0.5), 1.0);
	vec4 eqColor = vec4(0.2 + 0.5 * sin(UNIT_VECTOR.x) * sin(time * 0.334), 0.2 + 0.3 * cos(time * 0.434), 0.2, 1.0);
	
	// EQ bands.
	float bandIntensity0 = texture(fft, 0.25).x;
	float bandIntensity1 = texture(fft, 0.35).x;
	float bandIntensity2 = texture(fft, 0.65).x;
	float bandIntensity3 = texture(fft, 0.15).x;
	
	const vec4 bandTint0 = vec4(0.8, 0.5, 0.9, 1.0);
	const vec4 bandTint2 = vec4(0.4, 0.5, 0.9, 1.0);
	
	float eq = 0.5 + drawEq() * 0.5;
	
	color =
		// Base background color.
		baseColor * 0.7
		
		// EQ and color.
		+ vec4(eq / 2.0, eq / 3.0, eq, 0.0) * eqColor
		
		// Flare.
		+ drawFlare((bandIntensity1 + bandIntensity2) / 2.0)
		
		// Reverse flare, which becomes a circular shadow.
		- (1.0 - drawFlare(bandIntensity0 + bandIntensity1 + bandIntensity2 + bandIntensity3)) * 0.03
		
		// Rings.
		+ drawRing(0.50 * bandIntensity0, 0.005) * 0.75 * bandIntensity0 * bandTint0
		+ drawDistortedRing(0.40 * bandIntensity1, 0.05) * 0.45 * bandIntensity1
		+ drawRing(0.30 * bandIntensity2, 0.005) * 0.65 * bandIntensity2 * bandTint2
		+ drawDistortedRing(0.50 * bandIntensity3, 0.20) * 0.20 * bandIntensity3;
}

float drawEq() {
	float x = texture(fft, UNIT_VECTOR.x).x;
	float y = UNIT_VECTOR.y - 1.337;
	
	float intensity = cos(x) + y;
	intensity /= 0.001;
	
	intensity = 1.0 - intensity;
	return clamp(intensity, 0.0, 1.0);
}

float drawFlare(float intensity) {
	return smoothstep(0.3, 1.0, (1.0 - distance(UNIT_VECTOR_ASPECT, vec2(circleX, circleY))) * intensity);	
}

float drawDistortedRing(float radius0, float sharpness) {

	const float RING_THICKNESS = 0.015;
	float radius1 = radius0 - RING_THICKNESS;
	
	float a = atan(UNIT_VECTOR_ASPECT.x - 0.5, UNIT_VECTOR_ASPECT.y - 0.5);
	a = sin(a);
	
	float d = distance(UNIT_VECTOR_ASPECT, vec2(circleX, circleY));
	vec4 tv = texture(fft, a*a);
	float at = (sin(tv.x) + cos(tv.y)) / 2.0;
	return smoothstep(radius0 + sharpness, radius0, d * at)
		 * smoothstep(radius1 - sharpness, radius1, d * at);
}

float drawRing(float radius0, float sharpness) {

	const float RING_THICKNESS = 0.010;
	float radius1 = radius0 - RING_THICKNESS;
	
	float d = distance(UNIT_VECTOR_ASPECT, vec2(circleX, circleY));
	return smoothstep(radius0 + sharpness, radius0, d)
		 * smoothstep(radius1 - sharpness, radius1, d);
}
