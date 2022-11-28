#include "tree/grid.h"
#include <iostream>

// Commonly used names
using std::vector;
using std::array;
using std::tuple;
using glm::vec3;
using glm::ivec3;

// TODO DELETE FOR DEBUG
void print_vec3(vec3 pos){std::cout<<pos.x<<" "<<pos.y<<" "<<pos.z<<std::endl;}
// TODO DELETE FOR DEBUG

Grid::Grid(
        ivec3 dimensions, 
        float scale, 
        vec3 center
        ):
    dimensions(dimensions),
    grid(dimensions.x, vector<vector<unsigned int>>(dimensions.y, vector<unsigned int>(dimensions.z,0))),
    scale(scale),
    center(center),
    back_bottom_left(center-vec3(dimensions)*(scale/2.f)),
    grid_geom(vector<Vertex>(),vector<GLuint>()),
    occupied_geom(vector<Vertex>(),vector<GLuint>())
{
    gen_grid_geom();
}

ivec3 Grid::pos_to_grid(vec3 pos){ return ivec3((pos-back_bottom_left)/scale); }
vec3 Grid::grid_to_pos(ivec3 voxel){ return vec3(voxel)*scale+back_bottom_left;}

bool Grid::is_in_grid(ivec3 grid_cell)
{
    return  grid_cell.x>=0  && grid_cell.x<grid.size()&&
            grid_cell.y>=0  && grid_cell.y<grid[0].size()&&
            grid_cell.z>=0  && grid_cell.z<grid[0][0].size();
}

unsigned int Grid::get_in_grid(ivec3 index)
{
    if(!is_in_grid(index)) {std::cout<<"outside of grid"<<std::endl; return 0;}
    return grid[index.x][index.y][index.z];
}

unsigned int Grid::get_in_pos(vec3 pos){
    return get_in_grid(pos_to_grid(pos));
}

void Grid::occupy_pos(vec3 pos, unsigned int val){
    ivec3 slot = pos_to_grid(pos); 
    occupy_slot(slot, val);
}

void Grid::occupy_slot(ivec3 slot, unsigned int val){
    if(!is_in_grid(slot)) {std::cout<<"outside of grid"<<std::endl; return;}
    grid[slot.x][slot.y][slot.z] = val;
}

void Grid::occupy_line(vec3 start, vec3 end, unsigned int val)
{
    // fill voxel
    vector<ivec3> voxel_list = get_voxels_line(start,end);
    for (auto voxel : voxel_list){
        occupy_slot(voxel, val);
    }
}

vector<ivec3> Grid::get_voxels_line(vec3 start, vec3 end)
{
    // Initialize voxel list
    vector<ivec3> voxel_list;
    voxel_list.push_back(pos_to_grid(start));

    // Credit to: tlonny for this algo
    // https://gamedev.stackexchange.com/questions/72120/how-do-i-find-voxels-along-a-ray    
    // Initialize Position/Voxel Cursor and Direction vector
    vec3 pos_cursor = start;
    ivec3 voxel_cursor = pos_to_grid(pos_cursor);
    vec3 dir = end - start;
    float length2 = glm::length2(dir);
    if (length2 == 0.f) return voxel_list;
    dir = glm::normalize(dir);
    
    // Precompute direction components
    // tuple = (face normal index, component of direction, sign of face)
    vector<tuple<unsigned int, float, int>> dir_components;
    for(int i = 0; i < face_norms.size(); i++){
        float dir_comp = glm::dot(dir, vec3(face_norms[i]));
        if (dir_comp > 0){  // Only add components going in direction of normal
            int face_sign = face_norms[i].x + face_norms[i].y + face_norms[i].z;
            dir_components.push_back(std::make_tuple(i,dir_comp,face_sign));
        }
    }

    // loop while position is on line
    while(glm::distance2(start,pos_cursor)<length2&&is_in_grid(voxel_cursor)){
        float min_m = FLT_MAX;
        ivec3 min_norm;

        // Iterate through positive direction components
        for (int i = 0; i<dir_components.size(); i++){
            ivec3 norm = face_norms[std::get<0>(dir_components[i])];
            int norm_sign = std::get<2>(dir_components[i]);
 
            // Get position of next voxel boundary according to normal
            ivec3 next_voxel = voxel_cursor + norm;
            vec3 next_voxel_pos = grid_to_pos(next_voxel);
            if(norm_sign < 0) next_voxel_pos += vec3(scale,scale,scale)*0.9999f; 
            
            // Get difference in position for axis parallel to normal
            vec3 diff_pos = next_voxel_pos - pos_cursor;
            float component_diff = fabs(
                diff_pos.x*abs(norm.x)+
                diff_pos.y*abs(norm.y)+
                diff_pos.z*abs(norm.z));

            // Get distance needed to travel component_diff along the direction
            float m = component_diff/std::get<1>(dir_components[i]);

            if (m < min_m){
                if (m==0.f) m = FLT_MIN; // Fixes already being on border
                min_m = m;
                min_norm = norm;
            }
        }

        // Update cursor and position
        pos_cursor += dir * min_m; //scale to not land right on border
        voxel_cursor += min_norm;

        voxel_list.push_back(voxel_cursor);
    }
    // Preventing overshoot
    if (!glm::all(glm::equal(pos_to_grid(end),voxel_list.back()))||!is_in_grid(voxel_list.back())) voxel_list.pop_back(); 
    return voxel_list;
}

// TODO: IMPLEMENT WITH INSTANCING INSTEAD
// Generates all the geometry
void Grid::gen_occupied_geom()
{
    vector<Vertex>& vertices = occupied_geom.vertices;
    vector<GLuint>& indices = occupied_geom.indices;

    // Needs to be in same order as normals
    const array<array<GLuint, 6>,6> cube_indices= {{
        {5,1,3, 5,3,7}, // Right
        {0,4,6, 0,6,2}, // Left
        {6,7,3, 6,3,2}, // Top
        {4,5,1, 4,1,0}, // Bottom
        {4,5,7, 4,7,6}, // Front
        {1,0,3, 1,3,2}  // Back
    }};

    // Set up default vertex info array and adjacent vertex array
    // TODO: no need for structure, an int array is just fine
    /*
    struct Info{
        unsigned int index;
        bool vert_included=true;
    };
    const array<Info,8> default_verts_info = {{{0},{1},{2},{3},{4},{5},{6},{7}}};
    */
    const array<array<unsigned int,3>,8> adj_norms = {{
        {1,3,5},
        {0,3,5},
        {1,2,5},
        {0,2,5},
        {1,3,4},
        {0,3,4},
        {1,2,4},
        {0,2,4}
    }};

    vec3 vert_col = vec3(0.f,0.f,1.f);
    int current_index=0;

    // Loop Through all grid slots
    for(int k=0;k<grid[0][0].size();k++){
        for(int j=0;j<grid[0].size();j++){
            for(int i=0;i<grid.size();i++){
                ivec3 current_voxel(i,j,k);

                // Grid space is occupied
                if(get_in_grid(current_voxel)){ 
                    // Get adjacent voxel contents
                    vector<unsigned int> adj_content;
                    for ( auto norm : face_norms)
                        adj_content.push_back(get_in_grid(current_voxel+norm));

                    // Loop through and generate vertices
                    vec3 current_pos = back_bottom_left + vec3(current_voxel)*scale;
                    int curr_vert_index = 0;
                    for (int k_ = 0; k_<=1;k_++){
                        for (int j_ = 0; j_<=1;j_++){
                            for (int i_ = 0; i_<=1;i_++){
                                // TODO: remove vert if all 3 adjacent faces are occupies
                                vertices.push_back(Vertex{current_pos+vec3(i_,j_,k_)*scale,vert_col}); 
                                curr_vert_index++;
                            }
                        }
                    }

                    // Generate Indices
                    for (int face_i=0; face_i<cube_indices.size(); face_i++){
                        if (!adj_content[face_i]){
                            for (int index=0; index<cube_indices[face_i].size(); index++){
                                indices.push_back(cube_indices[face_i][index]+current_index);
                            }
                        }
                    }

                    current_index=vertices.size();
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
    vector<Vertex>& vertices = grid_geom.vertices;
    vector<GLuint>& indices = grid_geom.indices;

    //Vertex 
    float depth = center.z + dimensions.z*scale/2.f;
    float length = 0;
    vec3 vert_col = vec3(0.f,0.f,0.f);
    for (int j = 0; j <= dimensions.y; j++){
        for (int i = 0; i <= dimensions.x; i++){
            vec3 curr_pos = back_bottom_left+vec3(i,j,0)*scale;
            Vertex curr = {curr_pos,vert_col};
            vertices.push_back(curr);
            // ADD midpoints if on edge
            if (i==0 || i==dimensions.x || j==0 || j==dimensions.y){
                for (int k = 1; k<dimensions.z;k++){
                    vec3 next_in_pos = curr_pos+vec3(0.f,0.f,k)*scale;
                    Vertex next_in = {next_in_pos, vert_col};
                    vertices.push_back(next_in);
                }
            }
            vec3 next_pos = vec3(curr_pos.x,curr_pos.y,depth);
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
