#include <screenshot.h>
#include <glad/gl.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <filesystem>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <vector>

std::string makeScreenshotPath()
{
    namespace fs = std::filesystem;
    fs::path dir = "screenshots";
    std::error_code ec;
    fs::create_directories(dir, ec);

    std::time_t t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream stampStream;
    stampStream << std::put_time(&tm, "%Y%m%d_%H%M%S");
    const std::string stamp = stampStream.str();
    fs::path path = dir / (stamp + ".png");
    for (int suffix = 1; fs::exists(path); ++suffix)
        path = dir / (stamp + "_" + std::to_string(suffix) + ".png");
    return path.string();
}

bool saveScreenshotPNG(const std::string& path, int width, int height)
{
    if (width <= 0 || height <= 0) return false;

    std::vector<unsigned char> pixels(static_cast<size_t>(width) * height * 3);

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

    stbi_flip_vertically_on_write(1);
    return stbi_write_png(path.c_str(), width, height, 3, pixels.data(), width * 3) != 0;
}
