#version 330

uniform sampler2D textureMap0; //globalnie
uniform sampler2D textureMap1;
uniform int numLights; 

in vec4 iC;
in vec4 l[8];       // światła: l[0] = l1, l[1] = l2
in float distance[8]; // odległości: distance[0] = distance1, distance[1] = distance2
in vec4 n;
in vec4 v;
in vec2 iTexCoord0;
in vec2 iTexCoord1;
in float distance1;
in float distance2;


out vec4 pixelColor; //Zmienna wyjsciowa fragment shadera. Zapisuje sie do niej ostateczny (prawie) kolor piksela

void main(void) {

	//vec4 ml1 = normalize(l1);
	vec4 mn = normalize(n);
	vec4 mv = normalize(v);
	//vec4 mr1=reflect(-ml1,mn); //Wektor odbity
	//vec4 ml2 = normalize(l2); //Drugi wektor światła
	//vec4 mr2=reflect(-ml2,mn); //Drugi wektor odbity

	vec4 Kd=texture(textureMap0,iTexCoord0);

	
    vec4 Ks = vec4(1.0, 0.6, 0.2, 1.0); // podstawowy kolor odbicia
    
    // Bardzo subtelna pomarańczowa poświata
    vec4 orangeGlow = vec4(1.0, 0.8, 0.4, 1.0);

	float a = 1.0;
    float b = 0.1;
    float c = 0.01;

	 vec4 finalColor = vec4(0.0);

	 for (int i = 0; i < 8; i++) {
		if (i >= numLights) break;
		vec4 ml = normalize(l[i]);
        vec4 mr = reflect(-ml, mn);

        float nl = clamp(dot(mn, ml), 0.0, 1.0);
        float rv = pow(clamp(dot(mr, mv), 0.0, 1.0), 25.0);

        float attenuation = 1.0 / (a + b * distance[i] + c * distance[i] * distance[i]);

		// Oblicz siłę odbicia specular dla poświaty
        float specularStrength = rv * attenuation;

        vec4 diffuse = vec4(nl * Kd.rgb, Kd.a) / distance[i];
        vec4 specular = vec4(Ks.rgb * rv * attenuation, 0.0);
        
        // Bardzo subtelna pomarańczowa poświata bazująca na sile odbicia
        vec4 subtleGlow = vec4(orangeGlow.rgb * specularStrength * 0.15, 0.0); 

        finalColor += diffuse; // + specular +  + subtleGlow
	 }
	

	pixelColor = finalColor;



	//pixelColor = (color1)*attenuation1 + (color2)*attenuation2; //Wyliczenie modelu oświetlenia (bez ambient);
}


// matowa reflekcja

//vec4 ml = normalize(l);
//vec4 mn = normalize(n);
//vec4 mv = normalize(v);
//vec4 mr=reflect(-ml,mn); //Wektor odbity
//float nl = clamp(dot(mn, ml), 0, 1); //Kosinus kąta pomiędzy wektorami n i l.
//float rv = pow(clamp(dot(mr, mv), 0, 1), 25); // Kosinus kąta pomiędzy wektorami r i v podniesiony do 25 potęgi
//
//vec4 Kd=texture(textureMap0,iTexCoord0);
////vec4 Ks=vec4(Kd.rgb/3, 1);
//vec4 Ks=texture(textureMap1,iTexCoord0); // do odbicia
//
//pixelColor= vec4(nl * Kd.rgb, Kd.a) + vec4(Ks.rgb*rv, 0); //Wyliczenie modelu oświetlenia (bez ambient);
//