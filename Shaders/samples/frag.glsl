#version 430
uniform vec2 resolution;
uniform float time;
uniform sampler1D samples;
uniform sampler1D fft;
out vec4 color;
void main()
{
	vec2 uv = gl_FragCoord.xy / resolution.xy;
	int tx = int(uv.x*1024.0);

	float wave = texelFetch( samples, tx, 0 ).x / 2.0 + 0.5;
	float next = texelFetch( samples, tx+1, 0 ).x / 2.0 + 0.5;
	float prev = texelFetch( samples, tx-1, 0 ).x / 2.0 + 0.5;

	if (prev == 0.0f) prev = wave;
	if (next == 0.0f) next = wave;

	vec3 col;
	if ((prev - 0.01f <= uv.y && uv.y <= next + 0.01f) || (prev + 0.01f >= uv.y && uv.y >= next - 0.01f)){
        	col = vec3(uv, 0.0);
    	}


	color = vec4(col, 1.0f);
}
