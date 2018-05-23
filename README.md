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
  TODO
