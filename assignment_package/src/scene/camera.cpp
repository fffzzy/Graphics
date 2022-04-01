#include "camera.h"

Camera::Camera(glm::vec3 pos)
    : Camera(400, 400, pos)
{}

Camera::Camera(unsigned int w, unsigned int h, glm::vec3 pos)
    : Entity(pos), m_fovy(45), m_width(w), m_height(h),
      m_near_clip(0.1f), m_far_clip(1000.f),
      m_aspect(w / static_cast<float>(h)),
      theta(0.f), phi(0.f), zoom(1.f)
{}

Camera::Camera(const Camera &c)
    : Entity(c),
      m_fovy(c.m_fovy),
      m_width(c.m_width),
      m_height(c.m_height),
      m_near_clip(c.m_near_clip),
      m_far_clip(c.m_far_clip),
      m_aspect(c.m_aspect),
      theta(c.theta), phi(c.phi), zoom(c.zoom)
{}


void Camera::setWidthHeight(unsigned int w, unsigned int h) {
    m_width = w;
    m_height = h;
    m_aspect = w / static_cast<float>(h);
}


void Camera::tick(float dT, InputBundle &input) {
    // Do nothing
}

glm::mat4 Camera::getViewProj() const {
    return glm::perspective(glm::radians(m_fovy), m_aspect, m_near_clip, m_far_clip) * glm::lookAt(m_position, m_position + m_forward, m_up);
}

void Camera::RecomputeAttributes(int mouseX, int mouseY) {
    this->theta = fmod((((mouseX / static_cast<float>(m_width))) * (- 360.f) + 360.f), 360.f);
    this->phi = glm::clamp(((2.f * (mouseY / static_cast<float>(m_height * 0.95f)) - 1.f) * 90.f), -90.f, 90.f);
    m_forward = glm::vec3(glm::rotate(glm::mat4(), glm::radians(theta), glm::vec3(0.f, 1.f, 0.f))
                * glm::rotate(glm::mat4(), glm::radians(phi), glm::vec3(1.f, 0.f, 0.f)) * glm::vec4(0.f, 0.f, 1.f, 1.f));
    m_right = glm::vec3(glm::rotate(glm::mat4(), glm::radians(theta), glm::vec3(0.f, 1.f, 0.f))
                * glm::rotate(glm::mat4(), glm::radians(phi), glm::vec3(1.f, 0.f, 0.f)) * glm::vec4(1.f, 0.f, 0.f, 1.f));
    m_up = glm::vec3(glm::rotate(glm::mat4(), glm::radians(theta), glm::vec3(0.f, 1.f, 0.f))
                * glm::rotate(glm::mat4(), glm::radians(phi), glm::vec3(1.f, 0.f, 0.f)) * glm::vec4(0.f, 1.f, 0.f, 1.f));
}

glm::vec3 Camera::getForward() const {
    return this->m_forward;
}
