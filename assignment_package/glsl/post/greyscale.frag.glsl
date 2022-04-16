#version 150


out vec4 color;

//uniform sampler2D u_RenderedTexture;

void main()
{
    //vec4 C = texture(u_RenderedTexture, fs_UV);

    // TODO Homework 5
    //float dist = sqrt(pow(fs_UV[0]-0.5,2) + pow(fs_UV[1]-0.5,2));
    //float gray = 0.21*C[0] + 0.72*C[1] + 0.07*C[2];
    //float vignette = (sqrt(0.5) - dist)/(sqrt(0.5));
    //gray*= vignette;
    color = vec4(1.0,0.0,0.0,1.0);
}
