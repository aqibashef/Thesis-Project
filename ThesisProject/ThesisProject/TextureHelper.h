#pragma once

class TextureHelper
{
public:

	GLuint textures[1];


	TextureHelper();
	~TextureHelper();

	void bind(Mat texImage);
	GLuint getTextureId();
};

