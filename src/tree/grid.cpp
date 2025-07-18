#include "tree/grid.h"
#include "glm/geometric.hpp"
#include "glm/gtx/dual_quaternion.hpp"
#include "rendering/mesh.h"
#include "tree/implicit.h"
#include "util/geometry.h"
#include <climits>
#include <cmath>
#include <glm/gtx/io.hpp>
#include <iostream>
#include <omp.h>

// Commonly used names
using glm::ivec3;
using glm::vec3;
//using std::array;
using std::tuple;
using std::vector;

Grid::Grid(const Skeleton &tree, float percent_overshoot, float scale_factor) {
    vec3 bounds_size = tree.get_bounds().second - tree.get_bounds().first;
    back_bottom_left = tree.get_bounds().first - (bounds_size * percent_overshoot);
    vec3 front_top_right = tree.get_bounds().second + (bounds_size * percent_overshoot);
    center = 0.5f*(back_bottom_left+front_top_right);
    scale = tree.get_average_length() * scale_factor;
    dimensions = glm::ceil((front_top_right - back_bottom_left) / scale);
    grid =  vector<vector<vector<vector<size_t>>>>(dimensions.x, 
            vector<vector<vector<size_t>>>(dimensions.y,
            vector<vector<size_t>>(dimensions.z)));
    eval_grid = vector<vector<vector<struct Eval>>>(dimensions.x, 
                vector<vector<struct Eval>>(dimensions.y,
                vector<struct Eval>(dimensions.z)));
    //occupied.reserve(5000000);
    std::cout << "Grid dimensions: " << dimensions << std::endl;
}

ivec3 Grid::pos_to_grid(vec3 pos) const {
    return ivec3((pos - back_bottom_left) / scale);
}

vec3 Grid::grid_to_pos(ivec3 voxel) const {
    return vec3(voxel) * scale + back_bottom_left;
}

bool Grid::is_in_grid(ivec3 grid_cell) const {
    return  grid_cell.x >= 0 && grid_cell.x < dimensions.x && grid_cell.y >= 0 &&
            grid_cell.y < dimensions.y && grid_cell.z >= 0 &&
            grid_cell.z < dimensions.z;
}

// TODO: remove this function
float Grid::get_in_grid(ivec3 index) const {
    // REGULAR GRID
    if (!is_in_grid(index)) {
        return 0.f;
    }
    //FIXME: CHANGE MARKER
    //return grid[index.x][index.y][index.z];
    return 0.f;
}

bool Grid::has_refs(ivec3 index) const {
    // REGULAR GRID
    if (!is_in_grid(index)) {
        return false;
    }
    return !(grid[index.x][index.y][index.z].empty());
}

// TODO: remove this function
float Grid::get_in_pos(vec3 pos) const { return get_in_grid(pos_to_grid(pos)); }

float Grid::eval_pos(vec3 pos) const { 
    ivec3 slot = pos_to_grid(pos);
    if (!has_refs(slot)){
        return 0.f;
    }
    std::unordered_map<uint32_t, float> strand_vals;
    auto slot_refs = grid[slot.x][slot.y][slot.z];
    for (size_t i : slot_refs){
        // TODO: Extract to function
        struct Segment segment = segments[i]; 
        float v = segment.f.eval(pos, segment.start,segment.end);
        if (strand_vals.contains(segment.strand_id)){
            strand_vals[segment.strand_id] = 
                std::max(strand_vals[segment.strand_id], v);
        }else{
            strand_vals[segment.strand_id] = v;
        }
        //
    }
    float val = 0.f;
    for (auto v : strand_vals){
        val += v.second;
    }
    assert(val==val);
    return val;
}
float Grid::lazy_in_check(glm::ivec3 slot, float threshold){
    if (!is_in_grid(slot)) return 0.f;
    vec3 pos = grid_to_pos(slot);
    struct Eval& eval_slot = eval_grid[slot.x][slot.y][slot.z];
    std::vector<size_t>& strands = grid[slot.x][slot.y][slot.z];
    int i;
    while ((i = eval_slot.checked) < strands.size() && eval_slot.val <= threshold){
        // Eval based on next strand 
        int strand_i = strands[i];
        struct Segment segment = segments[strand_i]; 
        float v = segment.f.eval(pos,segment.start,segment.end);
        // Check strand val in dict 
        if (eval_slot.strands_checked.contains(segment.strand_id)){
            // If present and larger, replace value in dict and remove old val from lazy val 
            float old = eval_slot.strands_checked[segment.strand_id];
            eval_slot.val -= old;
            eval_slot.val += std::max(old,v);
            eval_slot.strands_checked[segment.strand_id]=std::max(old,v);
        }else{
            // If not present add to value and dict
            eval_slot.val+=v;
            eval_slot.strands_checked[segment.strand_id]=v;
        }
        eval_slot.checked++;
    }
    //std::cout <<strands.size()<<" "<<eval_slot.val<<std::endl;
    return eval_slot.val;
}
float Grid::lazy_eval(glm::ivec3 slot){
    return lazy_in_check(slot, FLT_MAX);
}
float Grid::get_texture_fac(glm::ivec3 slot){
    if (!is_in_grid(slot)) return 0.f;
    const int min=2;
    const int max=6;
    int num_strands = grid[slot.x][slot.y][slot.z].size();
    return std::clamp(
            (float)(num_strands-min)/(max-min)
            ,0.f,1.f);
}

glm::vec3 Grid::lazy_gradient(ivec3 slot){ 
    //float x = (eval_pos(slot - vec3(step_size, 0, 0)) - eval_pos(slot + vec3(step_size, 0, 0)));
    float x = (lazy_eval(slot - ivec3(1, 0, 0)) - lazy_eval(slot + ivec3(1, 0, 0)));
    float y = (lazy_eval(slot - ivec3(0, 1, 0)) - lazy_eval(slot + ivec3(0, 1, 0)));
    float z = (lazy_eval(slot - ivec3(0, 0, 1)) - lazy_eval(slot + ivec3(0, 0, 1)));
    assert(x==x);
    assert(y==y);
    assert(z==z);
    //assert(!(x==0.0f&&x==y&&x==z)); //FIXME 
    return glm::vec3(x,y,z);
}
glm::vec3 Grid::lazy_norm(ivec3 slot){ 
    return glm::normalize(lazy_gradient(slot));
}

glm::vec3 Grid::eval_norm(vec3 pos, float step_size) const { 
    return glm::normalize(eval_gradient(pos, step_size));
}

glm::vec3 Grid::eval_gradient(vec3 pos, float step_size) const { 
    float x = (eval_pos(pos - vec3(step_size, 0, 0)) - eval_pos(pos + vec3(step_size, 0, 0)));
    float y = (eval_pos(pos - vec3(0, step_size, 0)) - eval_pos(pos + vec3(0, step_size, 0)));
    float z = (eval_pos(pos - vec3(0, 0, step_size)) - eval_pos(pos + vec3(0, 0, step_size)));
    assert(x==x);
    assert(y==y);
    assert(z==z);
    //assert(!(x==0.0f&&x==y&&x==z)); //FIXME 
    return glm::vec3(x,y,z);
}

// FIXME: remove these 
glm::vec3 Grid::get_norm_grid(glm::ivec3 index) const {
    // Grid dependent normals
    return vec3(0,0,0);
    //return glm::normalize(glm::vec3((get_in_grid(index + ivec3(-1, 0, 0)) - get_in_grid(index + ivec3(1, 0, 0))), (get_in_grid(index + ivec3(0, -1, 0)) - get_in_grid(index + ivec3(0, 1, 0))), (get_in_grid(index + ivec3(0, 0, -1)) - get_in_grid(index + ivec3(0, 0, 1)))));
    //return glm::normalize(gradient[index.x][index.y][index.z]);
}
glm::vec3 Grid::get_norm_pos(glm::vec3 pos) const { 
    return get_norm_grid(pos_to_grid(pos)); 
}

bool Grid::line_occluded(glm::vec3 start, glm::vec3 end) {
    std::vector<glm::ivec3> voxels = get_voxels_line(start, end);
    if (voxels.size() == 0)
        return false;
    for (auto voxel : voxels) {
        if (get_in_grid(voxel) == 0)
            return false;
    }
    return true;
}

void Grid::occupy_pos(vec3 pos, float val) {
    ivec3 slot = pos_to_grid(pos);
    occupy_slot(slot, val);
}

void Grid::occupy_slot(ivec3 slot, float val) {
    if (!is_in_grid(slot)) {
        return;
    }
    // NORMAL GRID
    if (get_in_grid(slot) == 0) {
        //FIXME: CHANGE MARKER
        //grid[slot.x][slot.y][slot.z] = val;
        occupied.push_back(slot);
    }
}
void Grid::add_slot(ivec3 slot, float val) {
    // NORMAL GRID
    if (!is_in_grid(slot)) {
        return;
    }
    if (get_in_grid(slot) == 0) {
        occupied.push_back(slot);
    }
    //FIXME: CHANGE MARKER
    //grid[slot.x][slot.y][slot.z] += val;
}

void Grid::add_ref(glm::ivec3 slot, size_t segment) {
    // NORMAL GRID
    if (!is_in_grid(slot)) {
        return;
    }
    if (!has_refs(slot)) {
        occupied.push_back(slot);
    }
    grid[slot.x][slot.y][slot.z].push_back(segment);
}

void Grid::add_gradient(ivec3 slot, glm::vec3 val) {
}

void Grid::occupy_line(vec3 start, vec3 end, float val) {
    // fill voxel
    vector<ivec3> voxel_list = get_voxels_line(start, end);
    for (auto voxel : voxel_list) {
        occupy_slot(voxel, val);
    }
}

void Grid::occupy_path(std::vector<glm::vec3> path, float val) {
    if (path.size() < 2)
        return;
    for (int i = 0; i < path.size() - 1; i++) {
        occupy_line(path[i], path[i + 1], val);
    }
}
void Grid::fill_point(glm::vec3 p, Implicit &implicit) {
    int n = std::ceil(implicit.cutoff / scale);
    ivec3 slot = pos_to_grid(p);
    for (int i0 = -n; i0 <= n; ++i0) {
        for (int i1 = -n; i1 <= n; ++i1) {
            for (int i2 = -n; i2 <= n; ++i2) {
                ivec3 current_slot = slot + ivec3(i0, i1, i2);
                vec3 current_pos = grid_to_pos(current_slot);
                float val = implicit.eval(current_pos, p);
                if (val != 0) {
                    add_slot(current_slot, val);
                }
            }
        }
    }
}

void Grid::fill_line(uint32_t strand_id, glm::vec3 p1, glm::vec3 p2, MetaBalls &implicit) {
    struct Segment s = {.start = p1, .end = p2, .strand_id = strand_id, .f = implicit};
    segments.push_back(s);
    vec3 diff = p2 - p1;
    vec3 dir = glm::normalize(diff);
    vec3 variance = glm::abs(diff);
    vec3 segment_start = p1 - dir * implicit.cutoff;
    vec3 segment_end = p2 + dir * implicit.cutoff;

    int main_axis = 0;
    if (variance.y > variance.x && variance.y > variance.z) {
        main_axis = 1;
    } else if (variance.z > variance.x && variance.z > variance.y) {
        main_axis = 2;
    }
    int axis1 = (main_axis + 1) % 3;
    int axis2 = (main_axis + 2) % 3;
    glm::vec3 axis1_dir = glm::vec3(0,0,0);
    axis1_dir[axis1] = 1.0;
    glm::vec3 axis2_dir = glm::vec3(0,0,0);
    axis2_dir[axis2] = 1.0;
    int n = std::ceil((implicit.cutoff) / scale);
    int d1 = n,
        d2 = n;

    vector<ivec3> voxels = get_voxels_line(segment_start, segment_end);
    glm::ivec3 init_slot = pos_to_grid(segment_start);
    int last_main_axis = init_slot[main_axis];
    int last_axis1 = init_slot[axis1];
    int last_axis2 = init_slot[axis2];
    // Voxel Filling limits
    int l1_end = d1,
        l1_start = -d1,
        l2_end = d2,
        l2_start = -d2;
    for (int i = 0; i < voxels.size(); i++) {
        if (last_main_axis != voxels[i][main_axis]) { // Add all
            /*
            // TODO: Pushing to "occupied" vector messes this up
            int amount_to_add = (l1_end-l1_start)*(l2_end-l2_start);
            if (occupied.size()+amount_to_add > occupied.capacity()){
                occupied.reserve(occupied.size()*2);
                //occupied.reserve(occupied.size()+amount_to_add);
            }
            #pragma omp parallel for
            */
            for (int i1 = l1_start; i1 <= l1_end; i1++) {
                for (int i2 = l2_start; i2 <= l2_end; i2++) {
                    ivec3 slot_to_fill = voxels[i];
                    slot_to_fill[axis1] += i1;
                    slot_to_fill[axis2] += i2;
                    vec3 pos = grid_to_pos(slot_to_fill);
                    float val = implicit.eval(pos, p1, p2);
                    add_ref(slot_to_fill, segments.size()-1);
                }
            }
            l1_end = d1;
            l2_end = d2;
            l1_start = -d1;
            l2_start = -d2;
        } else { // Add extra
            // One of these should run
            if (last_axis1 != voxels[i][axis1]){
                if (last_axis1<voxels[i][axis1]){
                    l1_end++; 
                } else if (last_axis1>voxels[i][axis1]){
                    l1_start--; 
                }
            }else if (last_axis2 != voxels[i][axis2]){
                if (last_axis2<voxels[i][axis2]){
                    l2_end++; 
                } else if (last_axis2>voxels[i][axis2]){
                    l2_start--;
                }
            }
        }
        last_main_axis = voxels[i][main_axis];
        last_axis1 = voxels[i][axis1];
        last_axis2 = voxels[i][axis2];
    }
}

//FIXME: CHANGED THIS
void Grid::fill_path(std::vector<glm::vec3> path, Implicit& implicit){
    /*
    fill_line(0, path[0], path[1], implicit);
    for (int i = 1; i<path.size()-1; i++){
        fill_line(0, path[i], path[i + 1], implicit);
    }
    */
}
// THIS ONE
void Grid::fill_path(uint32_t strand_id, std::vector<glm::vec3> path, float max_val, float max_b, float shoot_b, float root_b, size_t inflection_point){
    for (int i = 0; i<path.size()-1; i++){
        float b;
        if (i<=inflection_point){
            b = std::lerp(shoot_b, max_b, (float)i/inflection_point);
        }else{
            b = std::lerp(max_b, root_b, (float)(i-inflection_point)/(path.size()-2-inflection_point));
        }
        MetaBalls implicit = MetaBalls(max_val, b);
        fill_line(strand_id, path[i], path[i + 1], implicit);
    }
}

// For making initial implicit field
// TODO: should start on tips then go down
float Grid::fill_skeleton(const Skeleton::Node& node, float min_range){ 
    float range = 0.f;
    if (node.children.empty()){
        range = min_range;
    }
    else{
        for (auto child : node.children){
            range = std::max(range,Grid::fill_skeleton(*child, min_range));
        }
    }
    //std::cout<<range<<" "<<min_range<<std::endl;
    if (node.parent != nullptr){  
        MetaBalls imp(3.0,range);
        fill_line(0, frame_position(node.frame),frame_position(node.parent->frame), imp);
    }
    return range+0.0001;
}

vector<ivec3> Grid::get_voxels_line(vec3 start, vec3 end) const {
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
    for (int i = 0; i < face_norms.size(); i++) {
        float dir_comp = glm::dot(dir, vec3(face_norms[i]));
        if (dir_comp > 0) { // Only add components going in direction of normal
            int face_sign = face_norms[i].x + face_norms[i].y + face_norms[i].z;
            dir_components.push_back(std::make_tuple(i, dir_comp, face_sign));
        }
    }

    // loop while position is on line
    while (glm::distance2(start, pos_cursor) < length2){
        float min_m = FLT_MAX;
        ivec3 min_norm;

        // Iterate through positive direction components
        for (int i = 0; i < dir_components.size(); i++) {
            ivec3 norm = face_norms[std::get<0>(dir_components[i])];
            int norm_sign = std::get<2>(dir_components[i]);

            // Get position of next voxel boundary according to normal
            ivec3 next_voxel = voxel_cursor + norm;
            vec3 next_voxel_pos = grid_to_pos(next_voxel);
            if (norm_sign < 0) next_voxel_pos += vec3(scale, scale, scale) * 0.9999f;

            // Get difference in position for axis parallel to normal
            vec3 diff_pos = next_voxel_pos - pos_cursor;
            float component_diff =
            fabs(diff_pos.x * abs(norm.x) + diff_pos.y * abs(norm.y) +
            diff_pos.z * abs(norm.z));

            // Get distance needed to travel component_diff along the direction
            float m = component_diff / std::get<1>(dir_components[i]);

            if (m < min_m) {
                if (m == 0.f) m = FLT_MIN; // Fixes already being on border
                min_m = m;
                min_norm = norm;
            }
        }

        // Update cursor and position
        pos_cursor += dir * min_m; // scale to not land right on border
        voxel_cursor += min_norm;
        voxel_list.push_back(voxel_cursor);
    }
    // Preventing overshoot
    if (!glm::all(glm::equal(pos_to_grid(end), voxel_list.back())) ||
        !is_in_grid(voxel_list.back())) voxel_list.pop_back();
    return voxel_list;
}

Mesh<VertFlat> Grid::get_occupied_geom_points(float threshold) const {
    const vec3 fullcol(1,0,0);
    const vec3 nocol(0,0,1);
    vector<VertFlat> vertices;
    vector<GLuint> indices;
    float max_val = 0.f;
    for (glm::ivec3 voxel : occupied) {
        float val = get_in_grid(voxel);
        if (val > max_val) {
            max_val = val;
        }
    }
    for (glm::ivec3 voxel : occupied) {
        float val = get_in_grid(voxel);
        if (val > threshold) {
            float intensity = val/max_val;
            vec3 col = (1-intensity)*nocol+intensity*fullcol;
            vec3 current_pos =
            back_bottom_left + vec3(voxel) * scale + vec3(1, 1, 1) * (scale / 2);
            vertices.push_back(VertFlat{current_pos, col});
        }
    }
    for (size_t i = 0; i < vertices.size(); i++) {
        indices.push_back(i);
    }
    return Mesh<VertFlat>(vertices, indices);
}

Mesh<VertFlat> Grid::get_normals_geom(float threshold) const {
    vector<VertFlat> vertices;
    vector<GLuint> indices;
    float max_val = 0.f;
    for (glm::ivec3 voxel : occupied) {
        if (get_in_grid(voxel) > threshold) {
            bool visible = false;
            for (auto norm : face_norms) {
                if (get_in_grid(voxel + norm) <= threshold) visible = true;
            }
            // Completely occluded do not add vertices
            if (!visible) continue;
            vec3 norm_start = back_bottom_left + vec3(voxel) * scale + vec3(1, 1, 1) * (scale / 2);
            vec3 norm_end = norm_start + get_norm_grid(voxel)*scale*2.0f;
            vertices.push_back(VertFlat{norm_start, glm::vec3(0,1,0)});
            vertices.push_back(VertFlat{norm_end, glm::vec3(0,1,0)});
        }
    }
    for (size_t i = 0; i < vertices.size(); i++) {
        indices.push_back(i);
    }
    return Mesh<VertFlat>(vertices, indices);
}

Mesh<Vertex> Grid::get_occupied_voxels(float threshold) const {
    vector<Vertex> vertices;
    vector<GLuint> indices;
    std::vector<glm::vec3> cube_verts{
        {0, 0, 0},          {scale, 0, 0},
        {0, scale, 0},      {scale, scale, 0},
        {0, 0, scale},      {scale, 0, scale},
        {0, scale, scale},  {scale, scale, scale},
    };
    std::vector<GLuint> cube_indices{
        1, 3, 7, 1, 7, 5, // Right
        0, 4, 6, 0, 6, 2, // Left
        4, 5, 7, 4, 7, 6, // Top
        0, 1, 3, 0, 3, 2, // Bottom
        0, 1, 5, 0, 5, 4, // Front
        2, 6, 7, 2, 7, 3  // Back
    };

    int current_index = 0;
    glm::vec3 col0 = glm::vec3(1,1,1);
    glm::vec3 col1 = glm::vec3(0.5,0,0.7);
    for (glm::ivec3 voxel : occupied) {
        // Grid space is occupied
        if (get_in_grid(voxel) >= threshold) {
            // Get adjacent voxel contents
            vector<float> adj_content;
            bool visible = false;
            for (auto norm : face_norms) {
                float content = get_in_grid(voxel + norm);
                adj_content.push_back(content);
                if (content <= threshold) visible = true;
            }
            // Completely occluded do not add vertices
            if (!visible) continue;

            glm::vec3 normal = get_norm_grid(voxel);
            //glm::vec3 normal = glm::normalize(gradient[voxel.x][voxel.y][voxel.z]);

            // Loop through and generate vertices
            vec3 current_pos = back_bottom_left + vec3(voxel) * scale;
            vec3 cube_col = random_color();
            for (int i_ = 0; i_ <= cube_verts.size(); i_++) {
                vertices.push_back(Vertex{current_pos + cube_verts[i_], random_brown(), normal});
            }
            // Generate Indices
            for (int i_ = 0; i_ < cube_indices.size(); i_++) {
                indices.push_back(current_index + cube_indices[i_]);
            }
            current_index = vertices.size();
        }
    }
    return Mesh(vertices, indices);
}

Mesh<Vertex> Grid::get_occupied_geom(float threshold,Grid& texture_space, std::pair<glm::vec3,glm::vec3>vis_bounds) {
    using namespace mc;
    if (vis_bounds.first==glm::vec3() && vis_bounds.second==glm::vec3()){
        vis_bounds.first = back_bottom_left;
        vis_bounds.second = back_bottom_left + scale*glm::vec3(dimensions);
    }
    vector<Vertex> verts;
    vector<GLuint> indices;
    for (auto occupied_slot : occupied){
        for (int z = -1; z <= 0; z++) { for (int y = -1; y <= 0; y++) { for (int x = -1; x <= 0; x++) {
            ivec3 offset(x, y, z);
            ivec3 voxel = occupied_slot + offset;
            vec3 voxel_pos = back_bottom_left+scale*glm::vec3(voxel);
            if (voxel_pos.x<vis_bounds.first.x||voxel_pos.x>vis_bounds.second.x||
                voxel_pos.y<vis_bounds.first.y||voxel_pos.y>vis_bounds.second.y||
                voxel_pos.z<vis_bounds.first.z||voxel_pos.z>vis_bounds.second.z) continue;
            if (voxel != occupied_slot && (!is_in_grid(voxel)||lazy_in_check(voxel,threshold) >= threshold))continue;

            ivec3 slots[8]={
                voxel+cell_order[0],
                voxel+cell_order[1],
                voxel+cell_order[2],
                voxel+cell_order[3],
                voxel+cell_order[4],
                voxel+cell_order[5],
                voxel+cell_order[6],
                voxel+cell_order[7],
            };
            vec3 cell_pos[8]={
                grid_to_pos(slots[0]),
                grid_to_pos(slots[1]),
                grid_to_pos(slots[2]),
                grid_to_pos(slots[3]),
                grid_to_pos(slots[4]),
                grid_to_pos(slots[5]),
                grid_to_pos(slots[6]),
                grid_to_pos(slots[7]),
            };
            if (lazy_in_check(slots[0],threshold) >= threshold &&
                lazy_in_check(slots[1],threshold) >= threshold &&
                lazy_in_check(slots[2],threshold) >= threshold &&
                lazy_in_check(slots[3],threshold) >= threshold &&
                lazy_in_check(slots[4],threshold) >= threshold &&
                lazy_in_check(slots[5],threshold) >= threshold &&
                lazy_in_check(slots[6],threshold) >= threshold &&
                lazy_in_check(slots[7],threshold) >= threshold) {
                    continue;
            }
            GridCell cell = {{
                { .pos=cell_pos[0], .val=lazy_eval(slots[0]), .norm=lazy_norm(slots[0]),.col_val=texture_space.lazy_eval(slots[0]) },
                { .pos=cell_pos[1], .val=lazy_eval(slots[1]), .norm=lazy_norm(slots[1]),.col_val=texture_space.lazy_eval(slots[1]) },
                { .pos=cell_pos[2], .val=lazy_eval(slots[2]), .norm=lazy_norm(slots[2]),.col_val=texture_space.lazy_eval(slots[2]) },
                { .pos=cell_pos[3], .val=lazy_eval(slots[3]), .norm=lazy_norm(slots[3]),.col_val=texture_space.lazy_eval(slots[3]) },
                { .pos=cell_pos[4], .val=lazy_eval(slots[4]), .norm=lazy_norm(slots[4]),.col_val=texture_space.lazy_eval(slots[4]) },
                { .pos=cell_pos[5], .val=lazy_eval(slots[5]), .norm=lazy_norm(slots[5]),.col_val=texture_space.lazy_eval(slots[5]) },
                { .pos=cell_pos[6], .val=lazy_eval(slots[6]), .norm=lazy_norm(slots[6]),.col_val=texture_space.lazy_eval(slots[6]) },
                { .pos=cell_pos[7], .val=lazy_eval(slots[7]), .norm=lazy_norm(slots[7]),.col_val=texture_space.lazy_eval(slots[7]) },
            }};
            polygonize(cell, threshold, verts, indices);
        }}}
    }
    std::cout<<"VERTS: " <<verts.size()<<" INDICES: "<<indices.size()<<std::endl;
    return Mesh<Vertex>(verts,indices);
}

Mesh<VertFlat> Grid::get_bound_geom() const {
    vector<VertFlat> vertices;
    vector<GLuint> indices = {0, 1, 2, 3, 4, 5, 6, 7, 0, 2, 1, 3,
                                4, 6, 5, 7, 0, 4, 1, 5, 2, 6, 3, 7};
    float width = dimensions.x * scale;
    float height = dimensions.y * scale;
    float depth = dimensions.z * scale;
    glm::vec3 col(0, 1, 0);
    for (int k = 0; k <= 1; k++) {
        for (int j = 0; j <= 1; j++) {
            for (int i = 0; i <= 1; i++) {
                vertices.push_back(VertFlat{
                back_bottom_left + vec3(i * width, j * height, k * depth), col});
            }
        }
    }
    return Mesh(vertices, indices);
}

Mesh<VertFlat> Grid::get_grid_geom() const {
    vector<VertFlat> vertices;
    vector<GLuint> indices;

    // Vertex
    float depth = center.z + dimensions.z * scale / 2.f;
    float length = 0;
    vec3 vert_col = vec3(0.f, 0.f, 0.f);
    for (int j = 0; j <= dimensions.y; j++) {
        for (int i = 0; i <= dimensions.x; i++) {
            vec3 curr_pos = back_bottom_left + vec3(i, j, 0) * scale;
            VertFlat curr = {curr_pos, vert_col};
            vertices.push_back(curr);
            // ADD midpoints if on edge
            if (i == 0 || i == dimensions.x || j == 0 || j == dimensions.y) {
                for (int k = 1; k < dimensions.z; k++) {
                    vec3 next_in_pos = curr_pos + vec3(0.f, 0.f, k) * scale;
                    VertFlat next_in = {next_in_pos, vert_col};
                    vertices.push_back(next_in);
                }
            }
            vec3 next_pos = vec3(curr_pos.x, curr_pos.y, depth);
            VertFlat next = {next_pos, vert_col};
            vertices.push_back(next);
        }
    }
    // Index
    int verts_in_layer = dimensions.x * 2 + dimensions.z * 2;
    int verts_first_layer = (dimensions.x + 1) * (dimensions.z + 1);
    // X grid lines
    for (int j = 0; j <= dimensions.y; j++) {
        int curr = (j > 0) ? verts_first_layer + (j - 1) * verts_in_layer : 0;
        for (int i = 0; i <= dimensions.x; i++) {
            indices.push_back(curr);
            int next = curr + 1;
            if (i == 0 || i == dimensions.x || j == 0 || j == dimensions.y) {
                next += dimensions.z - 1;
                curr += dimensions.z - 1;
            }
            indices.push_back(next);
            curr += 2;
        }
    }

    // Z grid lines
    for (int j = 0; j <= dimensions.y; j++) {
        int curr = (j > 0) ? verts_first_layer + (j - 1) * verts_in_layer : 0;
        for (int i = 0; i <= dimensions.z; i++) {
            indices.push_back(curr);
            int next = curr + dimensions.z + (dimensions.x - 1) * 2 + 1;
            if (j == 0 || j == dimensions.y) {
                next += (dimensions.x - 1) * (dimensions.z - 1);
            }
            indices.push_back(next);
            curr++;
        }
    }
    return Mesh(vertices, indices);
}

void Grid::export_data(const char *filename) {
    std::cout.flush();
    std::ofstream out(filename);
    for (glm::ivec3 voxel : occupied) {
        out << voxel.x << " " << voxel.y << " " << voxel.z << " "
            << get_in_grid(voxel) << "\n";
    }
    out.close();
}

// Marching Cubes
// BOURKE, P., 1994. Polygonising a Scalar Field (accessed on May 16, 2023). URL: http://paulbourke.net/geometry/polygonise/
void Grid::polygonize(const GridCell& cell, float threshold, vector<Vertex>& verts, vector<GLuint>& indices) const{
    using namespace mc;
    int i;
    int cubeindex;
    struct vert_index{
        Vertex v;
        GLuint i;
    };
    vert_index vertlist[12];

    // Find index
    cubeindex = 0;
    for (int i = 0; i < 8; i++) {
        if (cell[i].val > threshold) cubeindex |= 1 << i;
    }

    // Fully in/out of isosurface
    if (edge_table[cubeindex] == 0) return;

    // Add Vertices
    // TODO: Change this too loop instead of macros
    int N = 0, current_index = verts.size();
#define FINDVERT(A, B)                                                         \
    if (edge_table[cubeindex] & 1 << N) {                                      \
        vertlist[N].v = vertex_interp(threshold, cell[A], cell[B]);            \
        vertlist[N].i = current_index;                                         \
        verts.push_back(vertlist[N].v);                                        \
        current_index++;                                                       \
    }                                                                          \
    N++

    FINDVERT(0,1); FINDVERT(1,2); FINDVERT(2,3); FINDVERT(3,0); // Bottom Edges
    FINDVERT(4,5); FINDVERT(5,6); FINDVERT(6,7); FINDVERT(7,4); // Top Edges
    FINDVERT(0,4); FINDVERT(1,5); FINDVERT(2,6); FINDVERT(3,7); // Middle Edges

    // Add Indices
    for (i = 0; tri_table[cubeindex][i] != -1; i += 3) {
        indices.push_back(vertlist[tri_table[cubeindex][i]].i);
        indices.push_back(vertlist[tri_table[cubeindex][i+1]].i);
        indices.push_back(vertlist[tri_table[cubeindex][i+2]].i);
    }
}
Vertex Grid::vertex_interp(float threshold, const Grid::Sample& a, const Grid::Sample& b) const{
    vec3 col1(0,0,0);
    vec3 col0(.4,.2,.2);
    float min_col_val = 0.0f;
    float max_col_val = 15.0f;
    Vertex v = {vec3(),vec3(),vec3()};
    float col_factor;
    if (std::abs(threshold - a.val) < 0.00001) {
        v.position = a.pos;
        col_factor = a.col_val;
    } 
    else if (std::abs(threshold - b.val) < 0.00001) {
        v.position = b.pos;
        col_factor = b.col_val;
    } 
    else if (std::abs(threshold - b.val) < 0.00001) {
        v.position = a.pos;
        col_factor = a.col_val;
    }
    else {
        float mu = (threshold - a.val) / (b.val - a.val);
        v.position = a.pos + mu*(b.pos-a.pos);
        v.normal = a.norm + mu*(b.norm-a.norm);
        col_factor = a.col_val + mu*(b.col_val-a.col_val);
    }
    col_factor = std::clamp((col_factor-min_col_val)/(max_col_val-min_col_val),0.f,1.0f);
    v.color = (1.0f-col_factor)*col0+col_factor*col1;
    return v;
}

const ivec3 mc::cell_order[8] = {
    ivec3(1, 0, 0), ivec3(1, 0, 1), ivec3(0, 0, 1), ivec3(0, 0, 0),
    ivec3(1, 1, 0), ivec3(1, 1, 1), ivec3(0, 1, 1), ivec3(0, 1, 0),
};

const int mc::edge_table[256] = {
    0x0,   0x109, 0x203, 0x30a, 0x406, 0x50f, 0x605, 0x70c, 0x80c, 0x905, 0xa0f,
    0xb06, 0xc0a, 0xd03, 0xe09, 0xf00, 0x190, 0x99,  0x393, 0x29a, 0x596, 0x49f,
    0x795, 0x69c, 0x99c, 0x895, 0xb9f, 0xa96, 0xd9a, 0xc93, 0xf99, 0xe90, 0x230,
    0x339, 0x33,  0x13a, 0x636, 0x73f, 0x435, 0x53c, 0xa3c, 0xb35, 0x83f, 0x936,
    0xe3a, 0xf33, 0xc39, 0xd30, 0x3a0, 0x2a9, 0x1a3, 0xaa,  0x7a6, 0x6af, 0x5a5,
    0x4ac, 0xbac, 0xaa5, 0x9af, 0x8a6, 0xfaa, 0xea3, 0xda9, 0xca0, 0x460, 0x569,
    0x663, 0x76a, 0x66,  0x16f, 0x265, 0x36c, 0xc6c, 0xd65, 0xe6f, 0xf66, 0x86a,
    0x963, 0xa69, 0xb60, 0x5f0, 0x4f9, 0x7f3, 0x6fa, 0x1f6, 0xff,  0x3f5, 0x2fc,
    0xdfc, 0xcf5, 0xfff, 0xef6, 0x9fa, 0x8f3, 0xbf9, 0xaf0, 0x650, 0x759, 0x453,
    0x55a, 0x256, 0x35f, 0x55,  0x15c, 0xe5c, 0xf55, 0xc5f, 0xd56, 0xa5a, 0xb53,
    0x859, 0x950, 0x7c0, 0x6c9, 0x5c3, 0x4ca, 0x3c6, 0x2cf, 0x1c5, 0xcc,  0xfcc,
    0xec5, 0xdcf, 0xcc6, 0xbca, 0xac3, 0x9c9, 0x8c0, 0x8c0, 0x9c9, 0xac3, 0xbca,
    0xcc6, 0xdcf, 0xec5, 0xfcc, 0xcc,  0x1c5, 0x2cf, 0x3c6, 0x4ca, 0x5c3, 0x6c9,
    0x7c0, 0x950, 0x859, 0xb53, 0xa5a, 0xd56, 0xc5f, 0xf55, 0xe5c, 0x15c, 0x55,
    0x35f, 0x256, 0x55a, 0x453, 0x759, 0x650, 0xaf0, 0xbf9, 0x8f3, 0x9fa, 0xef6,
    0xfff, 0xcf5, 0xdfc, 0x2fc, 0x3f5, 0xff,  0x1f6, 0x6fa, 0x7f3, 0x4f9, 0x5f0,
    0xb60, 0xa69, 0x963, 0x86a, 0xf66, 0xe6f, 0xd65, 0xc6c, 0x36c, 0x265, 0x16f,
    0x66,  0x76a, 0x663, 0x569, 0x460, 0xca0, 0xda9, 0xea3, 0xfaa, 0x8a6, 0x9af,
    0xaa5, 0xbac, 0x4ac, 0x5a5, 0x6af, 0x7a6, 0xaa,  0x1a3, 0x2a9, 0x3a0, 0xd30,
    0xc39, 0xf33, 0xe3a, 0x936, 0x83f, 0xb35, 0xa3c, 0x53c, 0x435, 0x73f, 0x636,
    0x13a, 0x33,  0x339, 0x230, 0xe90, 0xf99, 0xc93, 0xd9a, 0xa96, 0xb9f, 0x895,
    0x99c, 0x69c, 0x795, 0x49f, 0x596, 0x29a, 0x393, 0x99,  0x190, 0xf00, 0xe09,
    0xd03, 0xc0a, 0xb06, 0xa0f, 0x905, 0x80c, 0x70c, 0x605, 0x50f, 0x406, 0x30a,
    0x203, 0x109, 0x0};

const int mc::tri_table[256][16] = {
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 1, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {1, 8, 3, 9, 8, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 8, 3, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {9, 2, 10, 0, 2, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {2, 8, 3, 2, 10, 8, 10, 9, 8, -1, -1, -1, -1, -1, -1, -1},
    {3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 11, 2, 8, 11, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {1, 9, 0, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {1, 11, 2, 1, 9, 11, 9, 8, 11, -1, -1, -1, -1, -1, -1, -1},
    {3, 10, 1, 11, 10, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 10, 1, 0, 8, 10, 8, 11, 10, -1, -1, -1, -1, -1, -1, -1},
    {3, 9, 0, 3, 11, 9, 11, 10, 9, -1, -1, -1, -1, -1, -1, -1},
    {9, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {4, 3, 0, 7, 3, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 1, 9, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {4, 1, 9, 4, 7, 1, 7, 3, 1, -1, -1, -1, -1, -1, -1, -1},
    {1, 2, 10, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {3, 4, 7, 3, 0, 4, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1},
    {9, 2, 10, 9, 0, 2, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1},
    {2, 10, 9, 2, 9, 7, 2, 7, 3, 7, 9, 4, -1, -1, -1, -1},
    {8, 4, 7, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {11, 4, 7, 11, 2, 4, 2, 0, 4, -1, -1, -1, -1, -1, -1, -1},
    {9, 0, 1, 8, 4, 7, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1},
    {4, 7, 11, 9, 4, 11, 9, 11, 2, 9, 2, 1, -1, -1, -1, -1},
    {3, 10, 1, 3, 11, 10, 7, 8, 4, -1, -1, -1, -1, -1, -1, -1},
    {1, 11, 10, 1, 4, 11, 1, 0, 4, 7, 11, 4, -1, -1, -1, -1},
    {4, 7, 8, 9, 0, 11, 9, 11, 10, 11, 0, 3, -1, -1, -1, -1},
    {4, 7, 11, 4, 11, 9, 9, 11, 10, -1, -1, -1, -1, -1, -1, -1},
    {9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {9, 5, 4, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 5, 4, 1, 5, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {8, 5, 4, 8, 3, 5, 3, 1, 5, -1, -1, -1, -1, -1, -1, -1},
    {1, 2, 10, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {3, 0, 8, 1, 2, 10, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1},
    {5, 2, 10, 5, 4, 2, 4, 0, 2, -1, -1, -1, -1, -1, -1, -1},
    {2, 10, 5, 3, 2, 5, 3, 5, 4, 3, 4, 8, -1, -1, -1, -1},
    {9, 5, 4, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 11, 2, 0, 8, 11, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1},
    {0, 5, 4, 0, 1, 5, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1},
    {2, 1, 5, 2, 5, 8, 2, 8, 11, 4, 8, 5, -1, -1, -1, -1},
    {10, 3, 11, 10, 1, 3, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1},
    {4, 9, 5, 0, 8, 1, 8, 10, 1, 8, 11, 10, -1, -1, -1, -1},
    {5, 4, 0, 5, 0, 11, 5, 11, 10, 11, 0, 3, -1, -1, -1, -1},
    {5, 4, 8, 5, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1},
    {9, 7, 8, 5, 7, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {9, 3, 0, 9, 5, 3, 5, 7, 3, -1, -1, -1, -1, -1, -1, -1},
    {0, 7, 8, 0, 1, 7, 1, 5, 7, -1, -1, -1, -1, -1, -1, -1},
    {1, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {9, 7, 8, 9, 5, 7, 10, 1, 2, -1, -1, -1, -1, -1, -1, -1},
    {10, 1, 2, 9, 5, 0, 5, 3, 0, 5, 7, 3, -1, -1, -1, -1},
    {8, 0, 2, 8, 2, 5, 8, 5, 7, 10, 5, 2, -1, -1, -1, -1},
    {2, 10, 5, 2, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1},
    {7, 9, 5, 7, 8, 9, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1},
    {9, 5, 7, 9, 7, 2, 9, 2, 0, 2, 7, 11, -1, -1, -1, -1},
    {2, 3, 11, 0, 1, 8, 1, 7, 8, 1, 5, 7, -1, -1, -1, -1},
    {11, 2, 1, 11, 1, 7, 7, 1, 5, -1, -1, -1, -1, -1, -1, -1},
    {9, 5, 8, 8, 5, 7, 10, 1, 3, 10, 3, 11, -1, -1, -1, -1},
    {5, 7, 0, 5, 0, 9, 7, 11, 0, 1, 0, 10, 11, 10, 0, -1},
    {11, 10, 0, 11, 0, 3, 10, 5, 0, 8, 0, 7, 5, 7, 0, -1},
    {11, 10, 5, 7, 11, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 8, 3, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {9, 0, 1, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {1, 8, 3, 1, 9, 8, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1},
    {1, 6, 5, 2, 6, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {1, 6, 5, 1, 2, 6, 3, 0, 8, -1, -1, -1, -1, -1, -1, -1},
    {9, 6, 5, 9, 0, 6, 0, 2, 6, -1, -1, -1, -1, -1, -1, -1},
    {5, 9, 8, 5, 8, 2, 5, 2, 6, 3, 2, 8, -1, -1, -1, -1},
    {2, 3, 11, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {11, 0, 8, 11, 2, 0, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1},
    {0, 1, 9, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1},
    {5, 10, 6, 1, 9, 2, 9, 11, 2, 9, 8, 11, -1, -1, -1, -1},
    {6, 3, 11, 6, 5, 3, 5, 1, 3, -1, -1, -1, -1, -1, -1, -1},
    {0, 8, 11, 0, 11, 5, 0, 5, 1, 5, 11, 6, -1, -1, -1, -1},
    {3, 11, 6, 0, 3, 6, 0, 6, 5, 0, 5, 9, -1, -1, -1, -1},
    {6, 5, 9, 6, 9, 11, 11, 9, 8, -1, -1, -1, -1, -1, -1, -1},
    {5, 10, 6, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {4, 3, 0, 4, 7, 3, 6, 5, 10, -1, -1, -1, -1, -1, -1, -1},
    {1, 9, 0, 5, 10, 6, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1},
    {10, 6, 5, 1, 9, 7, 1, 7, 3, 7, 9, 4, -1, -1, -1, -1},
    {6, 1, 2, 6, 5, 1, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1},
    {1, 2, 5, 5, 2, 6, 3, 0, 4, 3, 4, 7, -1, -1, -1, -1},
    {8, 4, 7, 9, 0, 5, 0, 6, 5, 0, 2, 6, -1, -1, -1, -1},
    {7, 3, 9, 7, 9, 4, 3, 2, 9, 5, 9, 6, 2, 6, 9, -1},
    {3, 11, 2, 7, 8, 4, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1},
    {5, 10, 6, 4, 7, 2, 4, 2, 0, 2, 7, 11, -1, -1, -1, -1},
    {0, 1, 9, 4, 7, 8, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1},
    {9, 2, 1, 9, 11, 2, 9, 4, 11, 7, 11, 4, 5, 10, 6, -1},
    {8, 4, 7, 3, 11, 5, 3, 5, 1, 5, 11, 6, -1, -1, -1, -1},
    {5, 1, 11, 5, 11, 6, 1, 0, 11, 7, 11, 4, 0, 4, 11, -1},
    {0, 5, 9, 0, 6, 5, 0, 3, 6, 11, 6, 3, 8, 4, 7, -1},
    {6, 5, 9, 6, 9, 11, 4, 7, 9, 7, 11, 9, -1, -1, -1, -1},
    {10, 4, 9, 6, 4, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {4, 10, 6, 4, 9, 10, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1},
    {10, 0, 1, 10, 6, 0, 6, 4, 0, -1, -1, -1, -1, -1, -1, -1},
    {8, 3, 1, 8, 1, 6, 8, 6, 4, 6, 1, 10, -1, -1, -1, -1},
    {1, 4, 9, 1, 2, 4, 2, 6, 4, -1, -1, -1, -1, -1, -1, -1},
    {3, 0, 8, 1, 2, 9, 2, 4, 9, 2, 6, 4, -1, -1, -1, -1},
    {0, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {8, 3, 2, 8, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1},
    {10, 4, 9, 10, 6, 4, 11, 2, 3, -1, -1, -1, -1, -1, -1, -1},
    {0, 8, 2, 2, 8, 11, 4, 9, 10, 4, 10, 6, -1, -1, -1, -1},
    {3, 11, 2, 0, 1, 6, 0, 6, 4, 6, 1, 10, -1, -1, -1, -1},
    {6, 4, 1, 6, 1, 10, 4, 8, 1, 2, 1, 11, 8, 11, 1, -1},
    {9, 6, 4, 9, 3, 6, 9, 1, 3, 11, 6, 3, -1, -1, -1, -1},
    {8, 11, 1, 8, 1, 0, 11, 6, 1, 9, 1, 4, 6, 4, 1, -1},
    {3, 11, 6, 3, 6, 0, 0, 6, 4, -1, -1, -1, -1, -1, -1, -1},
    {6, 4, 8, 11, 6, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {7, 10, 6, 7, 8, 10, 8, 9, 10, -1, -1, -1, -1, -1, -1, -1},
    {0, 7, 3, 0, 10, 7, 0, 9, 10, 6, 7, 10, -1, -1, -1, -1},
    {10, 6, 7, 1, 10, 7, 1, 7, 8, 1, 8, 0, -1, -1, -1, -1},
    {10, 6, 7, 10, 7, 1, 1, 7, 3, -1, -1, -1, -1, -1, -1, -1},
    {1, 2, 6, 1, 6, 8, 1, 8, 9, 8, 6, 7, -1, -1, -1, -1},
    {2, 6, 9, 2, 9, 1, 6, 7, 9, 0, 9, 3, 7, 3, 9, -1},
    {7, 8, 0, 7, 0, 6, 6, 0, 2, -1, -1, -1, -1, -1, -1, -1},
    {7, 3, 2, 6, 7, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {2, 3, 11, 10, 6, 8, 10, 8, 9, 8, 6, 7, -1, -1, -1, -1},
    {2, 0, 7, 2, 7, 11, 0, 9, 7, 6, 7, 10, 9, 10, 7, -1},
    {1, 8, 0, 1, 7, 8, 1, 10, 7, 6, 7, 10, 2, 3, 11, -1},
    {11, 2, 1, 11, 1, 7, 10, 6, 1, 6, 7, 1, -1, -1, -1, -1},
    {8, 9, 6, 8, 6, 7, 9, 1, 6, 11, 6, 3, 1, 3, 6, -1},
    {0, 9, 1, 11, 6, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {7, 8, 0, 7, 0, 6, 3, 11, 0, 11, 6, 0, -1, -1, -1, -1},
    {7, 11, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {3, 0, 8, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 1, 9, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {8, 1, 9, 8, 3, 1, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1},
    {10, 1, 2, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {1, 2, 10, 3, 0, 8, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1},
    {2, 9, 0, 2, 10, 9, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1},
    {6, 11, 7, 2, 10, 3, 10, 8, 3, 10, 9, 8, -1, -1, -1, -1},
    {7, 2, 3, 6, 2, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {7, 0, 8, 7, 6, 0, 6, 2, 0, -1, -1, -1, -1, -1, -1, -1},
    {2, 7, 6, 2, 3, 7, 0, 1, 9, -1, -1, -1, -1, -1, -1, -1},
    {1, 6, 2, 1, 8, 6, 1, 9, 8, 8, 7, 6, -1, -1, -1, -1},
    {10, 7, 6, 10, 1, 7, 1, 3, 7, -1, -1, -1, -1, -1, -1, -1},
    {10, 7, 6, 1, 7, 10, 1, 8, 7, 1, 0, 8, -1, -1, -1, -1},
    {0, 3, 7, 0, 7, 10, 0, 10, 9, 6, 10, 7, -1, -1, -1, -1},
    {7, 6, 10, 7, 10, 8, 8, 10, 9, -1, -1, -1, -1, -1, -1, -1},
    {6, 8, 4, 11, 8, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {3, 6, 11, 3, 0, 6, 0, 4, 6, -1, -1, -1, -1, -1, -1, -1},
    {8, 6, 11, 8, 4, 6, 9, 0, 1, -1, -1, -1, -1, -1, -1, -1},
    {9, 4, 6, 9, 6, 3, 9, 3, 1, 11, 3, 6, -1, -1, -1, -1},
    {6, 8, 4, 6, 11, 8, 2, 10, 1, -1, -1, -1, -1, -1, -1, -1},
    {1, 2, 10, 3, 0, 11, 0, 6, 11, 0, 4, 6, -1, -1, -1, -1},
    {4, 11, 8, 4, 6, 11, 0, 2, 9, 2, 10, 9, -1, -1, -1, -1},
    {10, 9, 3, 10, 3, 2, 9, 4, 3, 11, 3, 6, 4, 6, 3, -1},
    {8, 2, 3, 8, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1},
    {0, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {1, 9, 0, 2, 3, 4, 2, 4, 6, 4, 3, 8, -1, -1, -1, -1},
    {1, 9, 4, 1, 4, 2, 2, 4, 6, -1, -1, -1, -1, -1, -1, -1},
    {8, 1, 3, 8, 6, 1, 8, 4, 6, 6, 10, 1, -1, -1, -1, -1},
    {10, 1, 0, 10, 0, 6, 6, 0, 4, -1, -1, -1, -1, -1, -1, -1},
    {4, 6, 3, 4, 3, 8, 6, 10, 3, 0, 3, 9, 10, 9, 3, -1},
    {10, 9, 4, 6, 10, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {4, 9, 5, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 8, 3, 4, 9, 5, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1},
    {5, 0, 1, 5, 4, 0, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1},
    {11, 7, 6, 8, 3, 4, 3, 5, 4, 3, 1, 5, -1, -1, -1, -1},
    {9, 5, 4, 10, 1, 2, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1},
    {6, 11, 7, 1, 2, 10, 0, 8, 3, 4, 9, 5, -1, -1, -1, -1},
    {7, 6, 11, 5, 4, 10, 4, 2, 10, 4, 0, 2, -1, -1, -1, -1},
    {3, 4, 8, 3, 5, 4, 3, 2, 5, 10, 5, 2, 11, 7, 6, -1},
    {7, 2, 3, 7, 6, 2, 5, 4, 9, -1, -1, -1, -1, -1, -1, -1},
    {9, 5, 4, 0, 8, 6, 0, 6, 2, 6, 8, 7, -1, -1, -1, -1},
    {3, 6, 2, 3, 7, 6, 1, 5, 0, 5, 4, 0, -1, -1, -1, -1},
    {6, 2, 8, 6, 8, 7, 2, 1, 8, 4, 8, 5, 1, 5, 8, -1},
    {9, 5, 4, 10, 1, 6, 1, 7, 6, 1, 3, 7, -1, -1, -1, -1},
    {1, 6, 10, 1, 7, 6, 1, 0, 7, 8, 7, 0, 9, 5, 4, -1},
    {4, 0, 10, 4, 10, 5, 0, 3, 10, 6, 10, 7, 3, 7, 10, -1},
    {7, 6, 10, 7, 10, 8, 5, 4, 10, 4, 8, 10, -1, -1, -1, -1},
    {6, 9, 5, 6, 11, 9, 11, 8, 9, -1, -1, -1, -1, -1, -1, -1},
    {3, 6, 11, 0, 6, 3, 0, 5, 6, 0, 9, 5, -1, -1, -1, -1},
    {0, 11, 8, 0, 5, 11, 0, 1, 5, 5, 6, 11, -1, -1, -1, -1},
    {6, 11, 3, 6, 3, 5, 5, 3, 1, -1, -1, -1, -1, -1, -1, -1},
    {1, 2, 10, 9, 5, 11, 9, 11, 8, 11, 5, 6, -1, -1, -1, -1},
    {0, 11, 3, 0, 6, 11, 0, 9, 6, 5, 6, 9, 1, 2, 10, -1},
    {11, 8, 5, 11, 5, 6, 8, 0, 5, 10, 5, 2, 0, 2, 5, -1},
    {6, 11, 3, 6, 3, 5, 2, 10, 3, 10, 5, 3, -1, -1, -1, -1},
    {5, 8, 9, 5, 2, 8, 5, 6, 2, 3, 8, 2, -1, -1, -1, -1},
    {9, 5, 6, 9, 6, 0, 0, 6, 2, -1, -1, -1, -1, -1, -1, -1},
    {1, 5, 8, 1, 8, 0, 5, 6, 8, 3, 8, 2, 6, 2, 8, -1},
    {1, 5, 6, 2, 1, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {1, 3, 6, 1, 6, 10, 3, 8, 6, 5, 6, 9, 8, 9, 6, -1},
    {10, 1, 0, 10, 0, 6, 9, 5, 0, 5, 6, 0, -1, -1, -1, -1},
    {0, 3, 8, 5, 6, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {10, 5, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {11, 5, 10, 7, 5, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {11, 5, 10, 11, 7, 5, 8, 3, 0, -1, -1, -1, -1, -1, -1, -1},
    {5, 11, 7, 5, 10, 11, 1, 9, 0, -1, -1, -1, -1, -1, -1, -1},
    {10, 7, 5, 10, 11, 7, 9, 8, 1, 8, 3, 1, -1, -1, -1, -1},
    {11, 1, 2, 11, 7, 1, 7, 5, 1, -1, -1, -1, -1, -1, -1, -1},
    {0, 8, 3, 1, 2, 7, 1, 7, 5, 7, 2, 11, -1, -1, -1, -1},
    {9, 7, 5, 9, 2, 7, 9, 0, 2, 2, 11, 7, -1, -1, -1, -1},
    {7, 5, 2, 7, 2, 11, 5, 9, 2, 3, 2, 8, 9, 8, 2, -1},
    {2, 5, 10, 2, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1},
    {8, 2, 0, 8, 5, 2, 8, 7, 5, 10, 2, 5, -1, -1, -1, -1},
    {9, 0, 1, 5, 10, 3, 5, 3, 7, 3, 10, 2, -1, -1, -1, -1},
    {9, 8, 2, 9, 2, 1, 8, 7, 2, 10, 2, 5, 7, 5, 2, -1},
    {1, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 8, 7, 0, 7, 1, 1, 7, 5, -1, -1, -1, -1, -1, -1, -1},
    {9, 0, 3, 9, 3, 5, 5, 3, 7, -1, -1, -1, -1, -1, -1, -1},
    {9, 8, 7, 5, 9, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {5, 8, 4, 5, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1},
    {5, 0, 4, 5, 11, 0, 5, 10, 11, 11, 3, 0, -1, -1, -1, -1},
    {0, 1, 9, 8, 4, 10, 8, 10, 11, 10, 4, 5, -1, -1, -1, -1},
    {10, 11, 4, 10, 4, 5, 11, 3, 4, 9, 4, 1, 3, 1, 4, -1},
    {2, 5, 1, 2, 8, 5, 2, 11, 8, 4, 5, 8, -1, -1, -1, -1},
    {0, 4, 11, 0, 11, 3, 4, 5, 11, 2, 11, 1, 5, 1, 11, -1},
    {0, 2, 5, 0, 5, 9, 2, 11, 5, 4, 5, 8, 11, 8, 5, -1},
    {9, 4, 5, 2, 11, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {2, 5, 10, 3, 5, 2, 3, 4, 5, 3, 8, 4, -1, -1, -1, -1},
    {5, 10, 2, 5, 2, 4, 4, 2, 0, -1, -1, -1, -1, -1, -1, -1},
    {3, 10, 2, 3, 5, 10, 3, 8, 5, 4, 5, 8, 0, 1, 9, -1},
    {5, 10, 2, 5, 2, 4, 1, 9, 2, 9, 4, 2, -1, -1, -1, -1},
    {8, 4, 5, 8, 5, 3, 3, 5, 1, -1, -1, -1, -1, -1, -1, -1},
    {0, 4, 5, 1, 0, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {8, 4, 5, 8, 5, 3, 9, 0, 5, 0, 3, 5, -1, -1, -1, -1},
    {9, 4, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {4, 11, 7, 4, 9, 11, 9, 10, 11, -1, -1, -1, -1, -1, -1, -1},
    {0, 8, 3, 4, 9, 7, 9, 11, 7, 9, 10, 11, -1, -1, -1, -1},
    {1, 10, 11, 1, 11, 4, 1, 4, 0, 7, 4, 11, -1, -1, -1, -1},
    {3, 1, 4, 3, 4, 8, 1, 10, 4, 7, 4, 11, 10, 11, 4, -1},
    {4, 11, 7, 9, 11, 4, 9, 2, 11, 9, 1, 2, -1, -1, -1, -1},
    {9, 7, 4, 9, 11, 7, 9, 1, 11, 2, 11, 1, 0, 8, 3, -1},
    {11, 7, 4, 11, 4, 2, 2, 4, 0, -1, -1, -1, -1, -1, -1, -1},
    {11, 7, 4, 11, 4, 2, 8, 3, 4, 3, 2, 4, -1, -1, -1, -1},
    {2, 9, 10, 2, 7, 9, 2, 3, 7, 7, 4, 9, -1, -1, -1, -1},
    {9, 10, 7, 9, 7, 4, 10, 2, 7, 8, 7, 0, 2, 0, 7, -1},
    {3, 7, 10, 3, 10, 2, 7, 4, 10, 1, 10, 0, 4, 0, 10, -1},
    {1, 10, 2, 8, 7, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {4, 9, 1, 4, 1, 7, 7, 1, 3, -1, -1, -1, -1, -1, -1, -1},
    {4, 9, 1, 4, 1, 7, 0, 8, 1, 8, 7, 1, -1, -1, -1, -1},
    {4, 0, 3, 7, 4, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {4, 8, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {9, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {3, 0, 9, 3, 9, 11, 11, 9, 10, -1, -1, -1, -1, -1, -1, -1},
    {0, 1, 10, 0, 10, 8, 8, 10, 11, -1, -1, -1, -1, -1, -1, -1},
    {3, 1, 10, 11, 3, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {1, 2, 11, 1, 11, 9, 9, 11, 8, -1, -1, -1, -1, -1, -1, -1},
    {3, 0, 9, 3, 9, 11, 1, 2, 9, 2, 11, 9, -1, -1, -1, -1},
    {0, 2, 11, 8, 0, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {3, 2, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {2, 3, 8, 2, 8, 10, 10, 8, 9, -1, -1, -1, -1, -1, -1, -1},
    {9, 10, 2, 0, 9, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {2, 3, 8, 2, 8, 10, 0, 1, 8, 1, 10, 8, -1, -1, -1, -1},
    {1, 10, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {1, 3, 8, 9, 1, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 9, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 3, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}};
