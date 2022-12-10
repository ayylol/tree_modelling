#include "strands.h"

Strands::Strands(const Skeleton& tree, Grid& grid):
    grid(grid)
{
   for ( size_t i = 0; i<tree.leafs_size(); i++){
        paths.push_back(tree.get_strand(i));
   }
   // For testing
   for (auto path : paths){
        grid.occupy_path(path,1);
   }
}
Mesh Strands::get_mesh() const{
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
    for ( auto path : paths ) {
        glm::vec3 color = random_color();
        for ( auto position : path ) {
            vertices.push_back(Vertex{position, color});
        }
    }
    for (size_t i = 0; i < vertices.size(); i++){
        indices.push_back(i);
    }
    return Mesh(vertices, indices);
}
