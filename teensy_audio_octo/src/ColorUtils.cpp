#include <math.h>

// alternate code:
// http://forum.pjrc.com/threads/16469-looking-for-ideas-on-generating-RGB-colors-from-accelerometer-gyroscope?p=37170&viewfull=1#post37170
int HSVtoRGB(float H, float S,float V){
    if(H>360 || H<0 || S>100 || S<0 || V>100 || V<0)
    {
        return 0;
    }
    float s = S/100;
    float v = V/100;
    float C = s*v;
    float X = C*(1-fabsf(fmodf(H/60.0, 2)-1));
    float m = v-C;
    float r,g,b;
    if(H >= 0 && H < 60){
        r = C,g = X,b = 0;
    }
    else if(H >= 60 && H < 120){
        r = X,g = C,b = 0;
    }
    else if(H >= 120 && H < 180){
        r = 0,g = C,b = X;
    }
    else if(H >= 180 && H < 240){
        r = 0,g = X,b = C;
    }
    else if(H >= 240 && H < 300){
        r = X,g = 0,b = C;
    }
    else{
        r = C,g = 0,b = X;
    }

    unsigned int R = (r+m)*255;
    unsigned int G = (g+m)*255;
    unsigned int B = (b+m)*255;

    if (R > 255) R = 255;
    if (G > 255) G = 255;
    if (B > 255) B = 255;

    return (R << 16) | (G << 8) | B;
}