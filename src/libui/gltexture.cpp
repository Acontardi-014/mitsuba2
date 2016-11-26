#include <mitsuba/ui/gltexture.h>

NAMESPACE_BEGIN(mitsuba)

GLTexture::GLTexture() : m_id(0), m_index(0) { }
GLTexture::~GLTexture() { free(); }

void GLTexture::init(const Bitmap *bitmap) {
    if (m_id != 0)
        free();

    /* Generate an identifier */
    glGenTextures(1, &m_id);

    /* Bind to the texture */
    glBindTexture(GL_TEXTURE_2D, m_id);

    setInterpolation(EMipMapLinear);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    refresh(bitmap);
}

void GLTexture::free() {
    if (m_id == 0)
        return;
    glDeleteTextures(1, &m_id);
    m_id = 0;
}

void GLTexture::refresh(const Bitmap *bitmap) {
    GLenum format, internalFormat, type;

    switch (bitmap->componentFormat()) {
        case Struct::EInt8:    type = GL_BYTE; break;
        case Struct::EUInt8:   type = GL_UNSIGNED_BYTE; break;
        case Struct::EInt16:   type = GL_SHORT; break;
        case Struct::EUInt16:  type = GL_UNSIGNED_SHORT; break;
        case Struct::EInt32:   type = GL_INT; break;
        case Struct::EUInt32:  type = GL_UNSIGNED_INT; break;
        case Struct::EFloat16: type = GL_HALF_FLOAT; break;
        case Struct::EFloat32: type = GL_FLOAT; break;
        case Struct::EFloat64: type = GL_DOUBLE; break;
        default:
            Throw("GLTexture::refresh(): incompatible component format: %s",
                  bitmap->componentFormat());
    }

    switch (bitmap->pixelFormat()) {
        case Bitmap::ELuminance:      format = GL_RED; break;
        case Bitmap::ELuminanceAlpha: format = GL_RG; break;
        case Bitmap::ERGB:            format = GL_RGB; break;
        case Bitmap::ERGBA:           format = GL_RGBA; break;
        default:
            Throw("GLTexture::refresh(): incompatible pixel format: %s",
                  bitmap->pixelFormat());
    }
    internalFormat = format;

    if (bitmap->gamma() && bitmap->componentFormat() == Struct::EUInt8) {
        switch (bitmap->pixelFormat()) {
            case Bitmap::ERGB:            internalFormat = GL_SRGB8;
            case Bitmap::ERGBA:           internalFormat = GL_SRGB8_ALPHA8; break;
            default:
                Throw("GLTexture::refresh(): incompatible sRGB pixel format: %s",
                      bitmap->pixelFormat());
        }
    }

    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, (GLsizei) bitmap->width(),
                 (GLsizei) bitmap->height(), 0, format, type,
                 bitmap->data());
    glGenerateMipmap(GL_TEXTURE_2D);
}

void GLTexture::bind(int index) {
    glActiveTexture(GL_TEXTURE0 + index);
    glBindTexture(GL_TEXTURE_2D, m_id);
    m_index = index;
}

void GLTexture::setInterpolation(EInterpolation intp) {
    switch (intp) {
        case EMipMapLinear:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            break;

        case ENearest:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            break;

        case ELinear:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            break;

        default:
            Throw("GLTexture::setInterpolation(): invalid mode!");
    }
}

void GLTexture::release() {
    glActiveTexture(GL_TEXTURE0 + m_index);
    glBindTexture(GL_TEXTURE_2D, 0);
}

MTS_IMPLEMENT_CLASS(GLTexture, Object)
NAMESPACE_END(mitsuba)

