#include "color.h"
glm::vec3 random_color()
{
    const std::vector<glm::vec3> palette {{
        glm::vec3(0.58,0,0.83),
        glm::vec3(0.29,0,0.51),
        glm::vec3(0,0,1),
        glm::vec3(0,1,0),
        glm::vec3(1,1,0),
        glm::vec3(1,0.5,0),
        glm::vec3(1,0,0)
    }};
    return random_color(palette);
}
glm::vec3 random_brown()
{
    const std::vector<glm::vec3> palette {
        glm::vec3(0.1, 0.08, 0.01),
        glm::vec3(0.1, 0.08, 0.01),
        glm::vec3(0.1, 0.08, 0.01),
        glm::vec3(0.1, 0.08, 0.01),
        glm::vec3(0.1, 0.08, 0.01),
        glm::vec3(0.1, 0.08, 0.01),
        glm::vec3(0.09, 0.07, 0.01),
        glm::vec3(0.09, 0.07, 0.01),
        glm::vec3(0.12, 0.09, 0.04),
        glm::vec3(0.12, 0.09, 0.04),
    };
    return random_color(palette);
}
glm::vec3 random_color(const std::vector<glm::vec3>& palette)
{
    return palette[rand()%palette.size()];
}
