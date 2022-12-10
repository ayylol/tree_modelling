#include "strands.h"

Strands::Strands(const Skeleton& tree, Grid& grid):
    grid(grid)
{
   for ( size_t i = 0; i<tree.leafs_size(); i++){
        paths.push_back(tree.get_strand(i));
   }
}
Mesh Strands::get_mesh() const{
    // TODO: extract this to somewhere else pass in possibly?
    const std::array<glm::vec3,7> palette {{
        glm::vec3(0.58,0,0.83),
        glm::vec3(0.29,0,0.51),
        glm::vec3(0,0,1),
        glm::vec3(0,1,0),
        glm::vec3(1,1,0),
        glm::vec3(1,0.5,0),
        glm::vec3(1,0,0)
    }};
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
    for ( auto path : paths ) {
        glm::vec3 color = palette[(size_t)rand()%palette.size()];
        for ( auto position : path ) {
            vertices.push_back(Vertex{position, color});
        }
    }
    for (size_t i = 0; i < vertices.size(); i++){
        indices.push_back(i);
    }
    return Mesh(vertices, indices);
}
