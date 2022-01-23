#pragma once

#include "System.hpp"

class DisplaySystem;


//  The TexturedShapeSystem renders textured boxes.
//
class TexturedShapeSystem : public System {
public:
    ~TexturedShapeSystem() {};

    void init();
    void update(Components& components, DisplaySystem&);

private:
    unsigned int shader;
    unsigned int VAO;
    unsigned int VAOLength;

    unsigned int shaderModelLoc;
    unsigned int shaderViewLoc;
    unsigned int shaderProjectionLoc;
    unsigned int shaderAmbientStrengthLoc;
    unsigned int shaderDiffuseStrengthLoc;

    void init_shaders();
    void init_vao();
};
