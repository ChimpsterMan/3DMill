#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform bool hasTextures;
uniform sampler2D texture_diffuse1;

uniform vec3 diffuse_color;
uniform vec3 specular_color;
uniform vec3 ambient_color;

void main()
{   
	//texture management
	if (hasTextures){
    	FragColor = texture(texture_diffuse1, TexCoords);
    }
    else {
    	FragColor = vec4(1.0);
    }

    //material colors: diffuse
    FragColor *= vec4(diffuse_color, 1.0);
}