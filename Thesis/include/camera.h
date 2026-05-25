#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

struct CameraConfig {
    float fovY = glm::radians(45.f);
    float nearPlane = 0.1f;
    float farPlane = 1000.f;

    glm::vec3 target = glm::vec3(0.f);
    float radius = 5.f;
    float yaw = 0.f;       
    float pitch = 0.f;        
};

class Camera {
public:
    Camera(int viewportW, int viewportH, const CameraConfig& config = {});

    const glm::mat4& getViewMatrix()  ;
    const glm::mat4& getProjMatrix()  const;

    glm::vec3 getPosition()   const;
    glm::vec3 getForward()    const;
    float     getFovY()       const;
    float     getAspect()     const;

    void orbit(float deltYaw, float deltaPitch, float multi);
    void zoom(float delta, float sensitivity);
    void pan(float dx, float dy);
    void onViewportResize(int w, int h);


    bool needsSort() const;
    void onSortComplete();

private:
    void recompute();

    glm::vec3 target;
    float     radius;
    float     yaw;
    float     pitch;


    float fovY;
    float aspect;
    float nearPlane;
    float farPlane;


    glm::vec3 eye;
    glm::mat4 viewMatrix;
    glm::mat4 projMatrix;
    glm::vec3 lastSortPos;


    bool dirty = true;
};