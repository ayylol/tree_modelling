#include "color.h"
glm::vec3 random_color()
{
    const std::array<glm::vec3,7> palette {{
        glm::vec3(0.58,0,0.83),
        glm::vec3(0.29,0,0.51),
        glm::vec3(0,0,1),
        glm::vec3(0,1,0),
        glm::vec3(1,1,0),
        glm::vec3(1,0.5,0),
        glm::vec3(1,0,0)
    }};
    return palette[rand()%palette.size()];
}
