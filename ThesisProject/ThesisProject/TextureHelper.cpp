#include "stdafx.h"
#include "TextureHelper.h"


TextureHelper::TextureHelper()
{
}


TextureHelper::~TextureHelper()
{
}

void TextureHelper::bind(Mat texImage)
{
	glDeleteTextures(1, &textures[0]);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glEnable(GL_TEXTURE_2D);
	glGenTextures(1, &textures[0]);
	glBindTexture(GL_TEXTURE_2D, textures[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	/*glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);*/
	glTexImage2D(GL_TEXTURE_2D, 0, 3, texImage.cols, texImage.rows, 0, GL_RGB, GL_UNSIGNED_BYTE, texImage.data);
}

GLuint TextureHelper::getTextureId()
{
	return textures[0];
}
