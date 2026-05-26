#include <load_splat.h>

std::vector<RawSplat> loadRawSplats(const std::string& path)
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

    std::vector<RawSplat> splats(x.size());
    for (size_t i = 0; i < x.size(); i++)
    {
        splats[i].position[0] = x[i];
        splats[i].position[1] = y[i];
        splats[i].position[2] = z[i];

        splats[i].sh[0] = sh0[i];
        splats[i].sh[1] = sh1[i];
        splats[i].sh[2] = sh2[i];

        splats[i].opacity = opa[i];

        splats[i].scale[0] = sc0[i];
        splats[i].scale[1] = sc1[i];
        splats[i].scale[2] = sc2[i];

        splats[i].quaternion[0] = q0[i];
        splats[i].quaternion[1] = q1[i];
        splats[i].quaternion[2] = q2[i];
        splats[i].quaternion[3] = q3[i];
    }
    return splats;
}