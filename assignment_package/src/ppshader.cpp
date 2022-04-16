#include "ppshader.h"
#include <QDateTime>

PPShader::PPShader(OpenGLContext *context)
    : SProgram(context),
      attrPos(-1), attrUV(-1),
      unifDimensions(-1)
{}

PPShader::~PPShader()
{}

void PPShader::setupMemberVars()
{
    attrPos = context->glGetAttribLocation(prog, "vs_Pos");
    //attrUV  = context->glGetAttribLocation(prog, "vs_UV");

    unifTime = context->glGetUniformLocation(prog, "u_Time");
    unifSampler2D = context->glGetUniformLocation(prog, "u_RenderedTexture");
    unifDimensions = context->glGetUniformLocation(prog, "u_Dimensions");
}

void PPShader::draw(Drawable& d, int textureSlot = 0)
{
    GLenum error = glGetError();
    useMe();

    error = glGetError();
    // Set our "renderedTexture" sampler to user Texture Unit 0
    context->glUniform1i(unifSampler2D, textureSlot);

    // Each of the following blocks checks that:
    //   * This shader has this attribute, and
    //   * This Drawable has a vertex buffer for this attribute.
    // If so, it binds the appropriate buffers to each attribute.

    error = glGetError();
    if (attrPos != -1 && d.bindPos()) {
        context->glEnableVertexAttribArray(attrPos);
        context->glVertexAttribPointer(attrPos, 4, GL_FLOAT, false, 0, NULL);
    }
//    if (attrUV != -1 && d.bindUV()) {
//        context->glEnableVertexAttribArray(attrUV);
//        context->glVertexAttribPointer(attrUV, 2, GL_FLOAT, false, 0, NULL);
//    }

    // Bind the index buffer and then draw shapes from it.
    // This invokes the shader program, which accesses the vertex buffers.
    error = glGetError();
    d.bindIdx();
    error = glGetError();
    context->glDrawElements(d.drawMode(), d.elemCount(), GL_UNSIGNED_INT, 0);
    error = glGetError();

    if (attrPos != -1) context->glDisableVertexAttribArray(attrPos);
    error = glGetError();
    //if (attrNor != -1) context->glDisableVertexAttribArray(attrNor);
    //if (attrCol != -1) context->glDisableVertexAttribArray(attrCol);
    //if (attrUV != -1) context->glDisableVertexAttribArray(attrUV);

    error = glGetError();

    context->printGLErrorLog();
}


void PPShader::setDimensions(glm::ivec2 dims)
{
    useMe();

    if(unifDimensions != -1)
    {
        context->glUniform2i(unifDimensions, dims.x, dims.y);
    }
}
