#pragma once
struct Splat //236
{
    float position[3]; //->12
    float sh[3]; // -> 12
    float opacity;// -> 4
    float scale[3]; //-> 12
    float quaternion[4]; //[w,x,y,z] - > 16
    float sh_rest[45]; // -> 180 (SH bands 1-3, RGB-interleaved per basis coefficient)
};