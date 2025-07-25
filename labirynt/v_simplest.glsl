#version 330

//Zmienne jednorodne
uniform mat4 P;
uniform mat4 V;
uniform mat4 M;
uniform vec4 lp[8]; // pozycje dwóch źródeł światła jako tablica
uniform int numLights; 

//Atrybuty
in vec4 vertex; //wspolrzedne wierzcholka w przestrzeni modelu
in vec4 color; //kolor wierzchołka
in vec4 normal; //wektor normalny wierzchołka w przestrzeni modelu
in vec2 texCoord0;
in vec2 texCoord1;

out vec4 iC;
out vec4 l[8];         // wektory do światła
out vec4 n;
out vec4 v;
out vec2 iTexCoord0;
out vec2 iTexCoord1;
out float distance[8]; // odległości od źródeł światła

void main(void) {
    for (int i = 0; i < 8; i++) {
        if (i >= numLights) break;
        l[i] = normalize(V * (lp[i] - M * vertex)); // do światła w przestrzeni oka
        distance[i] = length(lp[i] - M * vertex);  // odległość od światła w przestrzeni świata
    }
    n = normalize(V * M * normal);//znormalizowany wektor normalny w przestrzeni oka
    v = normalize(vec4(0, 0, 0, 1) - V * M * vertex); //Wektor do obserwatora w przestrzeni oka
    
    iTexCoord1=(n.xy+1)/2;

    iC = color;
    iTexCoord0=texCoord0;

    gl_Position=P*V*M*vertex;
}


// DLA MATOWY
// l = normalize(V * (lp - M * vertex));//znormalizowany wektor do światła w przestrzeni oka
//    n = normalize(V * M * normal);//znormalizowany wektor normalny w przestrzeni oka
//    v = normalize(vec4(0, 0, 0, 1) - V * M * vertex); //Wektor do obserwatora w przestrzeni oka
//
//    iC = color;
//    iTexCoord0=texCoord0;
//
//    gl_Position=P*V*M*vertex;
