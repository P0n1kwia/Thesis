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

    auto r4 = tryGetProperty(plyData, "f_rest_3", n);
    auto r5 = tryGetProperty(plyData, "f_rest_4", n);
    auto r6 = tryGetProperty(plyData, "f_rest_5", n);
    auto r7 = tryGetProperty(plyData, "f_rest_6", n);
    auto r8 = tryGetProperty(plyData, "f_rest_7", n);
    auto g4 = tryGetProperty(plyData, "f_rest_18", n);
    auto g5 = tryGetProperty(plyData, "f_rest_19", n);
    auto g6 = tryGetProperty(plyData, "f_rest_20", n);
    auto g7 = tryGetProperty(plyData, "f_rest_21", n);
    auto g8 = tryGetProperty(plyData, "f_rest_22", n);
    auto b4 = tryGetProperty(plyData, "f_rest_33", n);
    auto b5 = tryGetProperty(plyData, "f_rest_34", n);
    auto b6 = tryGetProperty(plyData, "f_rest_35", n);
    auto b7 = tryGetProperty(plyData, "f_rest_36", n);
    auto b8 = tryGetProperty(plyData, "f_rest_37", n);

    auto r9  = tryGetProperty(plyData, "f_rest_8", n);
    auto r10 = tryGetProperty(plyData, "f_rest_9", n);
    auto r11 = tryGetProperty(plyData, "f_rest_10", n);
    auto r12 = tryGetProperty(plyData, "f_rest_11", n);
    auto r13 = tryGetProperty(plyData, "f_rest_12", n);
    auto r14 = tryGetProperty(plyData, "f_rest_13", n);
    auto r15 = tryGetProperty(plyData, "f_rest_14", n);
    auto g9  = tryGetProperty(plyData, "f_rest_23", n);
    auto g10 = tryGetProperty(plyData, "f_rest_24", n);
    auto g11 = tryGetProperty(plyData, "f_rest_25", n);
    auto g12 = tryGetProperty(plyData, "f_rest_26", n);
    auto g13 = tryGetProperty(plyData, "f_rest_27", n);
    auto g14 = tryGetProperty(plyData, "f_rest_28", n);
    auto g15 = tryGetProperty(plyData, "f_rest_29", n);
    auto b9  = tryGetProperty(plyData, "f_rest_38", n);
    auto b10 = tryGetProperty(plyData, "f_rest_39", n);
    auto b11 = tryGetProperty(plyData, "f_rest_40", n);
    auto b12 = tryGetProperty(plyData, "f_rest_41", n);
    auto b13 = tryGetProperty(plyData, "f_rest_42", n);
    auto b14 = tryGetProperty(plyData, "f_rest_43", n);
    auto b15 = tryGetProperty(plyData, "f_rest_44", n);

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
        splats[i].sh_rest[1] = g1[i];
        splats[i].sh_rest[2] = b1[i];
        splats[i].sh_rest[3] = r2[i];
        splats[i].sh_rest[4] = g2[i];
        splats[i].sh_rest[5] = b2[i];
        splats[i].sh_rest[6] = r3[i];
        splats[i].sh_rest[7] = g3[i];
        splats[i].sh_rest[8] = b3[i];

        splats[i].sh_rest[9] = r4[i];
        splats[i].sh_rest[10] = g4[i];
        splats[i].sh_rest[11] = b4[i];
        splats[i].sh_rest[12] = r5[i];
        splats[i].sh_rest[13] = g5[i];
        splats[i].sh_rest[14] = b5[i];
        splats[i].sh_rest[15] = r6[i];
        splats[i].sh_rest[16] = g6[i];
        splats[i].sh_rest[17] = b6[i];
        splats[i].sh_rest[18] = r7[i];
        splats[i].sh_rest[19] = g7[i];
        splats[i].sh_rest[20] = b7[i];
        splats[i].sh_rest[21] = r8[i];
        splats[i].sh_rest[22] = g8[i];
        splats[i].sh_rest[23] = b8[i];

        splats[i].sh_rest[24] = r9[i];
        splats[i].sh_rest[25] = g9[i];
        splats[i].sh_rest[26] = b9[i];
        splats[i].sh_rest[27] = r10[i];
        splats[i].sh_rest[28] = g10[i];
        splats[i].sh_rest[29] = b10[i];
        splats[i].sh_rest[30] = r11[i];
        splats[i].sh_rest[31] = g11[i];
        splats[i].sh_rest[32] = b11[i];
        splats[i].sh_rest[33] = r12[i];
        splats[i].sh_rest[34] = g12[i];
        splats[i].sh_rest[35] = b12[i];
        splats[i].sh_rest[36] = r13[i];
        splats[i].sh_rest[37] = g13[i];
        splats[i].sh_rest[38] = b13[i];
        splats[i].sh_rest[39] = r14[i];
        splats[i].sh_rest[40] = g14[i];
        splats[i].sh_rest[41] = b14[i];
        splats[i].sh_rest[42] = r15[i];
        splats[i].sh_rest[43] = g15[i];
        splats[i].sh_rest[44] = b15[i];
    }
    return splats;
}