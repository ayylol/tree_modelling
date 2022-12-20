#include "tree/grid.h"
#include <iostream>
#include <glm/gtx/io.hpp>

// Commonly used names
using std::vector;
using std::array;
using std::tuple;
using glm::vec3;
using glm::ivec3;

Grid::Grid(
        ivec3 dimensions, 
        float scale, 
        vec3 back_bottom_left
        ):
    dimensions(dimensions),
    grid(dimensions.x, vector<vector<float>>(dimensions.y, vector<float>(dimensions.z,0))),
    scale(scale),
    back_bottom_left(back_bottom_left),
    center(back_bottom_left+(scale/2.f)*(vec3)dimensions)
{
    // TESTING
    
    // TESTING
}
// Immplement
Grid::Grid(const Skeleton& tree, float percent_overshoot, float scale_factor)
{
    glm::vec3 bounds_size = tree.get_bounds().second - tree.get_bounds().first;
    back_bottom_left = tree.get_bounds().first - (bounds_size*percent_overshoot);
    glm::vec3 front_top_right = tree.get_bounds().second + (bounds_size*percent_overshoot);

    center = back_bottom_left+(scale/2.f)*(vec3)dimensions; // TODO REMOVE

    scale = tree.get_average_length()*scale_factor;
    dimensions = glm::ceil((front_top_right-back_bottom_left)/scale);
    grid = std::vector<std::vector<std::vector<float>>>(dimensions.x, vector<vector<float>>(dimensions.y, vector<float>(dimensions.z,0)));
}

ivec3 Grid::pos_to_grid(vec3 pos) const 
{ 
    return ivec3((pos-back_bottom_left)/scale); 
}

vec3 Grid::grid_to_pos(ivec3 voxel) const
{ 
    return vec3(voxel)*scale+back_bottom_left;
}

bool Grid::is_in_grid(ivec3 grid_cell) const
{
    return  grid_cell.x>=0  && grid_cell.x<grid.size()&&
            grid_cell.y>=0  && grid_cell.y<grid[0].size()&&
            grid_cell.z>=0  && grid_cell.z<grid[0][0].size();
}

float Grid::get_in_grid(ivec3 index) const
{
    if(!is_in_grid(index)) {
        //std::cout<<"outside of grid "<<index<<std::endl; 
        return 0;
    }
    return grid[index.x][index.y][index.z];
}

float Grid::get_in_pos(vec3 pos) const
{
    return get_in_grid(pos_to_grid(pos));
}

    
bool Grid::line_occluded(glm::vec3 start, glm::vec3 end){
    std::vector<glm::ivec3> voxels = get_voxels_line(start,end);
    if ( voxels.size() == 0 ) return false;
    for ( auto voxel : voxels ){
        if ( get_in_grid(voxel)==0 ) return false;
    }
    return true;
}

void Grid::occupy_pos(vec3 pos, float val){
    ivec3 slot = pos_to_grid(pos); 
    occupy_slot(slot, val);
}

void Grid::occupy_slot(ivec3 slot, float val){
    if(!is_in_grid(slot)) {
        //std::cout<<"outside of grid "<<slot<<std::endl;
        return;
    }
    if ( get_in_grid(slot) == 0 ){
        grid[slot.x][slot.y][slot.z] = val;
        occupied.push_back(slot);
    }
}

void Grid::occupy_line(vec3 start, vec3 end, float val)
{
    // fill voxel
    vector<ivec3> voxel_list = get_voxels_line(start,end);
    for (auto voxel : voxel_list){
        occupy_slot(voxel, val);
    }
}

void Grid::occupy_path(std::vector<glm::vec3> path, float val)
{
    if (path.size()<2) return;
    for (int i = 0; i < path.size()-1; i++){
        occupy_line(path[i],path[i+1],val);
    }
}

vector<ivec3> Grid::get_voxels_line(vec3 start, vec3 end) const
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


Mesh Grid::get_occupied_geom_points( float threshold ) const
{
    vector<Vertex> vertices;
    vector<GLuint> indices;
    for ( glm::ivec3 voxel : occupied ){
        if(get_in_grid(voxel) > threshold){ 
            vec3 current_pos = back_bottom_left + vec3(voxel)*scale + vec3(1,1,1)*(scale/2);
            vertices.push_back(Vertex{current_pos,random_color()});
            
        }
    }
    for (size_t i = 0; i < vertices.size(); i++){
        indices.push_back(i);
    }
    return Mesh(vertices,indices);
}

Mesh Grid::get_occupied_geom( float threshold ) const
{
    std::cout<<"Generating Occupied Geometry...";
    std::cout.flush();
    vector<Vertex> vertices;
    vector<GLuint> indices;

    glm::vec3 col(0,0,1);
    std::vector<std::pair<glm::vec3,glm::vec3>> cube_verts{
        // Right
        {glm::vec3(scale,scale,0),glm::vec3(1,0,0)}, {glm::vec3(scale,scale,scale),glm::vec3(1,0,0)}, 
        {glm::vec3(scale,0,0),glm::vec3(scale,0,0)}, {glm::vec3(scale,0,scale),glm::vec3(1,0,0)},
        // Left
        {glm::vec3(0,scale,0),glm::vec3(-1,0,0)}, {glm::vec3(0,scale,scale),glm::vec3(-1,0,0)}, 
        {glm::vec3(0,0,0),glm::vec3(-1,0,0)}, {glm::vec3(0,0,scale),glm::vec3(-1,0,0)},

        // Top
        {glm::vec3(0,scale,0),glm::vec3(0,1,0)}, {glm::vec3(scale,scale,0),glm::vec3(0,1,0)}, 
        {glm::vec3(0,scale,scale),glm::vec3(0,1,0)}, {glm::vec3(scale,scale,scale),glm::vec3(0,1,0)},
        // Bottom
        {glm::vec3(0,0,0),glm::vec3(0,-1,0)}, {glm::vec3(scale,0,0),glm::vec3(0,-1,0)}, 
        {glm::vec3(0,0,scale),glm::vec3(0,-1,0)}, {glm::vec3(scale,0,scale),glm::vec3(0,-1,0)},

        // Front
        {glm::vec3(0,scale,scale),glm::vec3(0,0,1)}, {glm::vec3(scale,scale,scale),glm::vec3(0,0,1)}, 
        {glm::vec3(0,0,scale),glm::vec3(0,0,1)}, {glm::vec3(scale,0,scale),glm::vec3(0,0,1)},
        // Back
        {glm::vec3(0,scale,0),glm::vec3(0,0,-1)}, {glm::vec3(scale,scale,0),glm::vec3(0,0,-1)}, 
        {glm::vec3(0,0,0),glm::vec3(0,0,-1)}, {glm::vec3(scale,0,0),glm::vec3(0,0,-1)},

    };
    std::vector<std::vector<GLuint>> cube_indices{
        {0,2,3, 0,3,1},         // Right
        {4,6,7, 4,7,5},         // Left
        {8,10,11, 8,11,9},      // Top
        {12,14,15, 12,15,13},   // Bottom
        {16,18,19, 16,19,17},   // Front
        {20,22,23, 20,23,21}    // Back
    };
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

    int current_index=0;
    for ( glm::ivec3 voxel : occupied ){
        // Grid space is occupied
        if(get_in_grid(voxel) > threshold){ 
            // Get adjacent voxel contents
            vector<float> adj_content;
            bool visible = false;
            for ( auto norm : face_norms){
                float content = get_in_grid(voxel+norm);
                adj_content.push_back(content);
                if( content == 0 ) visible = true;
            }
            if (!visible) continue; // Completely occluded do not add vertices

            // Loop through and generate vertices
            vec3 current_pos = back_bottom_left + vec3(voxel)*scale;
            int curr_vert_index = 0;
            //glm::vec3 color = random_color();
            //glm::vec3 color = random_brown();
            for ( int i_ = 0; i_<=cube_verts.size(); i_++){
                vertices.push_back(Vertex{current_pos+cube_verts[i_].first, random_brown(), cube_verts[i_].second});
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
    std::cout<<" Done"<<std::endl;
    return Mesh(vertices, indices);
}

Mesh Grid::get_bound_geom() const
{
    vector<Vertex> vertices;
    vector<GLuint> indices = {
        0,1,2,3,4,5,6,7,
        0,2,1,3,4,6,5,7,
        0,4,1,5,2,6,3,7
        };
    float width=dimensions.x*scale;
    float height=dimensions.y*scale;
    float depth=dimensions.z*scale;
    glm::vec3 col(0,1,0);
    for (int k = 0; k<=1;k++){
        for (int j = 0; j<=1;j++){
            for (int i = 0; i<=1;i++){
                vertices.push_back(
                        Vertex{back_bottom_left+vec3(i*width,j*height,k*depth),
                        col}); 
            }
        }
    }
    return Mesh(vertices,indices);
}

Mesh Grid::get_grid_geom() const
{
    vector<Vertex> vertices;
    vector<GLuint> indices;

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
    return Mesh(vertices, indices);
}
void Grid::export_data(const char * filename){
    std::cout<<"Exporting Data...";
    std::cout.flush();
    std::ofstream out(filename);
    for(int k=0;k<grid[0][0].size();k++){
        for(int j=0;j<grid[0].size();j++){
            for(int i=0;i<grid.size();i++){
                if(grid[i][j][k]!=0){
                    out<< i <<" "<< j << " "<< k <<" "<<(grid[i][j][k])<<"\n";
                }
            }
        }
    }
    out.close();
    std::cout<<" Done"<<std::endl;
}

void Grid::smooth_grid(){
    std::cout<<"Smoothing Grid...";
    std::cout.flush();
    std::vector<std::vector<std::vector<float>>> temp_grid = grid;
    // TODO Horrible unbelievable nesting
    for(int k=0;k<grid[0][0].size();k++){
        for(int j=0;j<grid[0].size();j++){
            for(int i=0;i<grid.size();i++){
                float total_around=0.0;
                for(int z=-1;z<=1;z++){
                    for(int y=-1;y<=1;y++){
                        for(int x=-1;x<=1;x++){
                            total_around += get_in_grid(glm::ivec3(i+x,i+y,i+z));
                        }
                    }
                }
                temp_grid[i][j][k]=total_around/27.f;
            }
        }
    }
    grid = temp_grid;
    std::cout<<" Done" <<std::endl;
}
