class RGB
{
public:
    int R;
    int G;
    int B;
};



class Cylinder
{
public:
    RGB colour(int y)
    {
        Vector _normalVector;

    }
private:
    int _radius;
    int _yCenter;
    RGB _ambientColour;
    RGB _diffuseColour;
    RGB _specularColour;
    Vector _lightVector;

};

colour = ambientColour + (diffuseColour*(lightVector dot normalVector) + specularColour*(reflectedLightVector dot viewVector)^n)

reflectedVector = lightVector - 2*(normalVector dot lightVector)*normalVector

Apropos of nothing, I really like the word birefringence


0 1 |    |4 5 |    |
    | 8 9|6   | C D|
2 3 |    |  7 |    |
    | A B|    | E F|

0 1 9 F
0 2 A F
0 3 B F
0 4 C F
0 5 D F
0 6 E F
0 8 7 F

Plan:
  Create Windows app to visualize raster bar patterns
  Initially render one large bar






L = light direction vector (unit)
V = viewing direction vector (unit)
H = (L + V)/2 normalized
Kd = wavelength-dependent empirical diffuse reflection coefficient
Ks = wavelength-dependent empirical specular reflection coefficient
SpecIndex = index that simulates surface roughness:
  infinity for a perfect mirror
IaKa = ambient light intensity * wavelength-dependent empirical ambient reflection coefficient
Ilight = light intensity

0.57735 0.57735 1.57735 length = 2/3 + 2.488 = 3.1547
0.183012 0.183012



void ShadeSphere(int SpecIndex, Vector centre, int radius)
{
}

ShadeSphere(0.85,0.15,10,160,100,100);


