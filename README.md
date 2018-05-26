# liveW
Live background wallpaper in opengl for i3. It's made for shaders from [Shadertoy.com](shadertoy.com).

# Requirements:
 - Pulseaudio
 - FFTW
 
# Compiling:
```
git clone https://github.com/dgranosa/liveW.git
cd liveW
make
```

# Using:
$ ./liveW -s _source_ -p _shader_name_

_source_ is name of Pulseaudio device which you can obtain with:
```
$ pacmd list-sources | grep "name:"
  name: <alsa_output.pci-0000_22_00.3.analog-stereo.monitor>
  name: <alsa_input.pci-0000_22_00.3.analog-stereo>
```
You are intrested in one with .monitor on the end.

_shader_name_ is name of the shaders folder inside folder Shaders/ in root directory of project.

Example: ./liveW -s alsa_output.pci-0000_22_00.3.analog-stereo.monitor -p equalizer
```
$ ./liveW -h
  Usage: liveW <options>                                                      
  Options:                                                                    
     -h Print help                                                            
     -d Turn debug on                                                         
     -w Window mode                                                           
     -g Window geometry WIDTHxHEIGHT (default 800x600)                        
     -t Transparency (default 0.8)                                            
     -p Shader name in Shaders folder                                         
     -f FPS (default 30)                                                      
     -s Pulseaudio device source                                              
        Specify using the name from "pacmd list-sources | grep "name:""
```
# Using shaders from Shadertoy
**NOTE: liveW have only support for music input for now**
  1. Create directory inside Shaders/ and copy shader code in file frag.glsl inside created folder.
  2. Add to begining of the shader:
   ```
   #version 430
   uniform vec2 resolution;
   uniform float time;
   uniform sampler1D samples;
   uniform sampler1D fft;
   out vec4 color;
   ```
  3. Rename possible colission between variables names from above inside code in something else.
  4. Replace:
     - iResolution -> resolution
     - fragCoord -> gl_FragCoord
     - fragColor -> color
     - iTime -> time
     - void mainImage( in, out ) -> void main()
  5. If using music input replace code:
     - texture(iChannel0, vec2(coordX, coordY)).x;
     - If coordY is smaller then 0.5 replace with:
       - texture(fft, coordX).x;
     - otherwise
       - texture(samples, coordX).x;
