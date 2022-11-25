#include "grid.h"
#include <iostream>

Grid::Grid(
        glm::ivec3 dimensions, 
        float scale, 
        glm::vec3 center
        ):
    dimensions(dimensions),
    grid(dimensions.x, std::vector<std::vector<unsigned int>>(dimensions.y, std::vector<unsigned int>(dimensions.z,0))),
    scale(scale),
    center(center),
    back_bottom_left(center-glm::vec3(dimensions)*(scale/2.f)),
    grid_geom(std::vector<Vertex>(),std::vector<GLuint>()),
    occupied_geom(std::vector<Vertex>(),std::vector<GLuint>())
{
    gen_grid_geom();
}

glm::ivec3 Grid::pos_to_grid(glm::vec3 pos){ return glm::ivec3((pos-back_bottom_left)/scale); }

bool Grid::is_in_grid(glm::ivec3 grid_cell)
{
    return  grid_cell.x>=0  && grid_cell.x<grid.size()&&
            grid_cell.y>=0  && grid_cell.y<grid[0].size()&&
            grid_cell.z>=0  && grid_cell.z<grid[0][0].size();
}

unsigned int Grid::get_in_grid(glm::ivec3 index)
{
    if(!is_in_grid(index)) {std::cout<<"outside of grid"<<std::endl; return 0;}
    return grid[index.x][index.y][index.z];
}

unsigned int Grid::get_in_pos(glm::vec3 pos){
    return get_in_grid(pos_to_grid(pos));
}

void Grid::occupy_pos(glm::vec3 pos, unsigned int val){
    glm::ivec3 slot = pos_to_grid(pos); 
    occupy_slot(slot, val);
}
void Grid::occupy_slot(glm::ivec3 slot, unsigned int val){
    if(!is_in_grid(slot)) {std::cout<<"outside of grid"<<std::endl; return;}
    grid[slot.x][slot.y][slot.z] = val;
}
    
void Grid::occupy_line(glm::vec3 start, glm::vec3 end, unsigned int val)
{
    // fill voxel
    std::vector<glm::ivec3> voxel_list = get_voxels_line(start,end);
    for (auto voxel : voxel_list){
        occupy_slot(voxel, val);
    }
}
std::vector<glm::ivec3> Grid::get_voxels_line(glm::vec3 start, glm::vec3 end)
{
    std::vector<glm::ivec3> voxel_list;
    voxel_list.push_back(pos_to_grid(start));
    voxel_list.push_back(pos_to_grid(end));
    // Initialize Cursor
    // Enumerate faces
    // loop while position is on line
    // Iterate through sides
    // Get component of direction
    // Find how far to travel before boundary is reached
    // Compare to m to min_m
    // Update cursor and position
    // Add voxel to return list
    return voxel_list;
}

void Grid::gen_occupied_geom()
{
    std::vector<Vertex>& vertices = occupied_geom.vertices;
    std::vector<GLuint>& indices = occupied_geom.indices;

    std::vector<GLuint> cube_indices{
        // Back
        1,0,3,
        1,3,2,
        // Left
        0,4,6,
        0,6,2,
        // Front
        4,5,7,
        4,7,6,
        // Right
        5,1,3,
        5,3,7,
        // Top
        6,7,3,
        6,3,2,
        // Bottom
        4,5,1,
        4,1,0
    };

    glm::vec3 vert_col = glm::vec3(0.f,0.f,1.f);
    int current_index=0;
    for(int k=0;k<grid[0][0].size();k++){
        for(int j=0;j<grid[0].size();j++){
            for(int i=0;i<grid.size();i++){
                if(grid[i][j][k]){
                    // Add cube at the position
                    // Generate vertices
                    // TODO: could be made into a function 
                    glm::vec3 curr = back_bottom_left + glm::vec3(i,j,k)*scale;
                    for (int k_ = 0; k_<=1;k_++){
                        for (int j_ = 0; j_<=1;j_++){
                            for (int i_ = 0; i_<=1;i_++){
                                vertices.push_back(Vertex{curr+glm::vec3(i_,j_,k_)*scale,vert_col});
                            }
                        }
                    }
                    std::vector<GLuint> new_indices = cube_indices;
                    std::for_each(new_indices.begin(), new_indices.end(),[&current_index](GLuint &n){n+=current_index;}); 
                    indices.insert(indices.end(),new_indices.begin(),new_indices.end());
                    current_index=vertices.size();
                    //std::cout<<i<<" "<<j<<" "<<k<<std::endl;
                }
            }
        }
    }
    //indices = cube_indices;
    //for (int i=0; i<vertices.size();i++){
        //indices.push_back(i);
    //}
    occupied_geom.update();
}

void Grid::gen_grid_geom()
{
    std::vector<Vertex>& vertices = grid_geom.vertices;
    std::vector<GLuint>& indices = grid_geom.indices;

    //Vertex 
    float depth = center.z + dimensions.z*scale/2.f;
    float length = 0;
    glm::vec3 vert_col = glm::vec3(0.f,0.f,0.f);
    for (int j = 0; j <= dimensions.y; j++){
        for (int i = 0; i <= dimensions.x; i++){
            glm::vec3 curr_pos = back_bottom_left+glm::vec3(i,j,0)*scale;
            Vertex curr = {curr_pos,vert_col};
            vertices.push_back(curr);
            // ADD midpoints if on edge
            if (i==0 || i==dimensions.x || j==0 || j==dimensions.y){
                for (int k = 1; k<dimensions.z;k++){
                    glm::vec3 next_in_pos = curr_pos+glm::vec3(0.f,0.f,k)*scale;
                    Vertex next_in = {next_in_pos, vert_col};
                    vertices.push_back(next_in);
                }
            }
            glm::vec3 next_pos = glm::vec3(curr_pos.x,curr_pos.y,depth);
            Vertex next = {next_pos,vert_col};
            vertices.push_back(next);
        }
    }
    // Index
    int verts_in_layer = dimensions.x*2+dimensions.z*2;
    int verts_first_layer = (dimensions.x+1)*(dimensions.z+1);
    // X grid lines
    for (int j=0; j<=dimensions.y; j++){
        int curr = (j>0) ? verts_first_layer+(j-1)*verts_in_layer : 0;
        for (int i=0; i<=dimensions.x; i++){
            indices.push_back(curr);
            int next = curr+1;
            if(i==0||i==dimensions.x||j==0||j==dimensions.y){
                next += dimensions.z-1;
                curr += dimensions.z-1;
            }
            indices.push_back(next);
            curr+=2;
        }
    }

    // Z grid lines
    for (int j=0;j<=dimensions.y; j++){
        int curr = (j>0) ? verts_first_layer+(j-1)*verts_in_layer : 0;
        for (int i=0;i<=dimensions.z; i++){
            indices.push_back(curr);
            int next = curr + dimensions.z + (dimensions.x-1)*2 + 1;
            if (j==0||j==dimensions.y){
                next+=(dimensions.x-1)*(dimensions.z-1);
            }
            indices.push_back(next);
            curr++; 
        }
    }
    /*
    // Y grid lines
    for (int i = 0; i < verts_first_layer; i++){
    indices.push_back(i);
    indices.push_back(i+(verts_in_layer)*(dimensions.y-1)+verts_first_layer);
    }
    */

    /*
       for (int i=0; i<vertices.size();i++){
       indices.push_back(i);
       }
       */
    grid_geom.update();

}
