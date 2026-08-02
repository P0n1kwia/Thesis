#include <load_splat.h>
#include <algorithm>
#include <cmath>

namespace
{
    std::vector<float> tryGetProperty(happly::PLYData & ply, const std::string& name, size_t n)
    {
        try
        {
            return ply.getElement("vertex").getProperty<float>(name);
        }
        catch (...)
        {
            return std::vector<float>(n, 0.0f);
        }
    }
}
std::vector<Splat> loadSplats(const std::string& path)
{
    happly::PLYData plyData(path);

    auto x = plyData.getElement("vertex").getProperty<float>("x");
    auto y = plyData.getElement("vertex").getProperty<float>("y");
    auto z = plyData.getElement("vertex").getProperty<float>("z");

    auto sh0 = plyData.getElement("vertex").getProperty<float>("f_dc_0");
    auto sh1 = plyData.getElement("vertex").getProperty<float>("f_dc_1");
    auto sh2 = plyData.getElement("vertex").getProperty<float>("f_dc_2");

    auto opa = plyData.getElement("vertex").getProperty<float>("opacity");

    auto sc0 = plyData.getElement("vertex").getProperty<float>("scale_0");
    auto sc1 = plyData.getElement("vertex").getProperty<float>("scale_1");
    auto sc2 = plyData.getElement("vertex").getProperty<float>("scale_2");

    auto q0 = plyData.getElement("vertex").getProperty<float>("rot_0");
    auto q1 = plyData.getElement("vertex").getProperty<float>("rot_1");
    auto q2 = plyData.getElement("vertex").getProperty<float>("rot_2");
    auto q3 = plyData.getElement("vertex").getProperty<float>("rot_3");

    size_t n = x.size();
    auto r1 = tryGetProperty(plyData, "f_rest_0", n);
    auto r2 = tryGetProperty(plyData, "f_rest_1", n);
    auto r3 = tryGetProperty(plyData, "f_rest_2", n);
    auto g1 = tryGetProperty(plyData, "f_rest_15", n);
    auto g2 = tryGetProperty(plyData, "f_rest_16", n);
    auto g3 = tryGetProperty(plyData, "f_rest_17", n);
    auto b1 = tryGetProperty(plyData, "f_rest_30", n);
    auto b2 = tryGetProperty(plyData, "f_rest_31", n);
    auto b3 = tryGetProperty(plyData, "f_rest_32", n);

    std::vector<Splat> splats(x.size());
    float SH_C0 = 0.28209479177f;
    for (size_t i = 0; i < x.size(); i++)
    {
        splats[i].position[0] = x[i];
        splats[i].position[1] = y[i];
        splats[i].position[2] = z[i];

        splats[i].sh[0] = sh0[i];
        splats[i].sh[1] = sh1[i];
        splats[i].sh[2] = sh2[i];
        
        splats[i].opacity = (1.0f) / (1.0f + std::exp((-opa[i])));

        splats[i].scale[0] = std::exp(sc0[i]);
        splats[i].scale[1] = std::exp(sc1[i]);
        splats[i].scale[2] = std::exp(sc2[i]);

        float len = std::sqrt(q0[i] * q0[i] + q1[i] * q1[i] + q2[i] * q2[i] + q3[i] * q3[i]);

        splats[i].quaternion[0] = q0[i]/len;
        splats[i].quaternion[1] = q1[i]/len;
        splats[i].quaternion[2] = q2[i]/len;
        splats[i].quaternion[3] = q3[i]/len;

        splats[i].sh_rest[0] = r1[i];
        splats[i].sh_rest[1] = r2[i];
        splats[i].sh_rest[2] = r3[i];

        splats[i].sh_rest[3] = g1[i];
        splats[i].sh_rest[4] = g2[i];
        splats[i].sh_rest[5] = g3[i];
        splats[i].sh_rest[6] = b1[i];
        splats[i].sh_rest[7] = b2[i];
        splats[i].sh_rest[8] = b3[i];
    }
    return splats;
}