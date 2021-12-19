# Dependencies

sudo apt install libglfw3 libglfw3-dev libglfw3-doc
sudo apt install libxi-dev
sudo apt install libglm-dev

# GLAD
Generate ZIP file at via http://glad.dav1d.de/ .  Then, unpack it in this directory.
It should produce files:
    src/glad.c
    include/glad/glad.h
    include/KHR/khrplatform.h
Also, `touch include/glad/glad.h include/KHR/khrplatform.h src/glad.c` if unzip creates the files with unreasonable timestamps.


# References
GLFW documentation at https://www.glfw.org/documentation.html .
Also, if libglfw3-doc is installed, file:///usr/share/doc/libglfw3-dev/html/index.html .
