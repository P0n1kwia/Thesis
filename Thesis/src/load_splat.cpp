#include <load_splat.h>
#include <algorithm>
#include <cmath>
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

    std::vector<Splat> splats(x.size());
    float SH_C0 = 0.28209479177f;
    for (size_t i = 0; i < x.size(); i++)
    {
        splats[i].position[0] = x[i];
        splats[i].position[1] = y[i];
        splats[i].position[2] = z[i];

        splats[i].sh[0] = std::clamp(0.5f + SH_C0 * sh0[i], 0.0f, 1.0f);
        splats[i].sh[1] = std::clamp(0.5f + SH_C0 * sh1[i], 0.0f, 1.0f);
        splats[i].sh[2] = std::clamp(0.5f + SH_C0 * sh2[i], 0.0f, 1.0f);
        
        splats[i].opacity = (1.0f) / (1.0f + std::exp((-opa[i])));

        splats[i].scale[0] = std::exp(sc0[i]);
        splats[i].scale[1] = std::exp(sc1[i]);
        splats[i].scale[2] = std::exp(sc2[i]);

        float len = std::sqrt(q0[i] * q0[i] + q1[i] * q1[i] + q2[i] * q2[i] + q3[i] * q3[i]);

        splats[i].quaternion[0] = q0[i]/len;
        splats[i].quaternion[1] = q1[i]/len;
        splats[i].quaternion[2] = q2[i]/len;
        splats[i].quaternion[3] = q3[i]/len;
    }
    return splats;
}