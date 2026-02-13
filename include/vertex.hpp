#pragma once

#include <array>

#include <cstddef>
#include <vulkan/vulkan.hpp>

#include <glm/glm.hpp>


struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;

    static vk::VertexInputBindingDescription getBindingDescription() {
        vk::VertexInputBindingDescription bindingDescription(0, sizeof(Vertex), vk::VertexInputRate::eVertex);
        return bindingDescription;
    }

    static std::array<vk::VertexInputAttributeDescription, 2> getAttributeDescriptions() {
        vk::VertexInputAttributeDescription posDescription(0, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, pos));
        vk::VertexInputAttributeDescription colorDescription(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color));

        return { posDescription, colorDescription};
    }

};