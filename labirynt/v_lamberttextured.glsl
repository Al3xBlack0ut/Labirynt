#version 330

//Zmienne jednorodne
uniform mat4 P;
uniform mat4 V;
uniform mat4 M;

//Atrybuty
layout (location=0) in vec4 vertex;    // Wsp�rz�dne wierzcho�ka w przestrzeni modelu
layout (location=1) in vec4 normal;    // Wektor normalny (nieu�ywany tutaj)
layout (location=2) in vec2 texCoord;  // Wsp�rz�dne teksturowania

//Zmienne interpolowane
out vec2 i_tc;
out float i_nl;

void main(void) {
    gl_Position = P * V * M * vertex;

    // Sta�e o�wietlenie - bez wzgl�du na orientacj� �ciany
    i_nl = 1.0;

    i_tc = texCoord;
}
