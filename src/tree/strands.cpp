#include "strands.h"
#include "glm/fwd.hpp"
#include "glm/geometric.hpp"
#include "glm/gtx/vector_angle.hpp"
#include "tree/implicit.h"
#include "util/geometry.h"
#include <glm/gtx/io.hpp>

std::default_random_engine
    rng(std::chrono::system_clock::now().time_since_epoch().count());

glm::vec3 random_vector(glm::vec3 axis, float angle);

Strands::Strands(const Skeleton &tree, Grid &grid, Implicit &evalfunc) : 
    grid(grid),evalfunc(evalfunc), tree(tree)
{
    for (size_t i = 0; i < tree.leafs_size(); i++) {
        shoot_paths.push_back(tree.get_strand(i, Skeleton::LEAF));
    }
    for (size_t i = 0; i < tree.roots_size(); i++) {
        std::vector<glm::vec3> root_path = tree.get_strand(i, Skeleton::ROOT);
        // Reverse the path
        for (size_t i = 0; i < root_path.size()/2; i++) {
            size_t j = root_path.size()-1-i;
            std::swap(root_path[i],root_path[j]);
        }
        root_paths.push_back(root_path);
    }

    // Initialize Root Angle Vectors
    root_vecs.reserve(root_paths.size());
    for (size_t i = 0; i<root_paths.size(); i++){
        // TODO: make the index used for the angle vector a parameter
        glm::vec3 angle_vec = root_paths[i][(root_paths[i].size()-1)/4] - tree.get_root_pos();
        angle_vec.y=0.f;
        angle_vec = glm::normalize(angle_vec);
        root_vecs.push_back(angle_vec);
    }
}
void Strands::add_strands(nlohmann::json& options){
    int num_strands = tree.leafs_size();
    if (options.contains("num_abs")) {
        num_strands = options.at("num_abs");
    } else if (options.contains("num_per")) {
        num_strands *= (int)options.at("num_per");
    }
    if (options.contains("sectorality")) {
        select_method = options.at("sectorality") ? WithAngle : AtRandom;
    }
    segment_length = options.at("segment_length");
    num_trials = options.at("num_trials");
    max_angle = options.at("max_angle");
    alpha = options.at("alpha");
    offset = options.at("segment_offset");
    reject_iso = options.at("reject_iso");
    add_strands(num_strands);
}
void Strands::add_strands(unsigned int amount) {
    std::vector<size_t> paths(shoot_paths.size());
    std::iota(paths.begin(),paths.end(),0);
    std::shuffle(paths.begin(), paths.end(), rng);
    for (size_t i = 0; i < amount; i++) {
        add_strand(paths[i%paths.size()]);
    }
    std::cout << "Strands Termniated: "<< strands_terminated << std::endl;
}

// THE ALGORITHM THAT IMPLEMENTS STRAND VOXEL AUTOMATA
// TODO: Extract to seperate smaller functions
void Strands::add_strand(size_t shoot_index) {
    if (shoot_index >= shoot_paths.size()){
        return;
    }

    // Set up strand
    const std::vector<glm::vec3> *path = &(shoot_paths[shoot_index]);
    size_t closest_index = 0;
    glm::vec3 last_closest = (*path)[closest_index];
    std::vector<glm::vec3> strand{(*path)[closest_index]};

    // Set up root path (if selectpos is at leaf)
    size_t root_index = 0;
    if (select_pos == AtLeaf){
        root_index = match_root(last_closest); 
    }

    // Loop until on root, and target node is the end
    bool on_root = false;
    bool done = false;
    while (!done) {
        // Start of this segment is head of last
        glm::vec3 start(strand[strand.size() - 1]);

        // Find Target point
        size_t target_index = closest_index;
        glm::vec3 target_point = (*path)[target_index];
        float travelled = 0.f;
        float distance_to_travel =
            segment_length + glm::distance(target_point, start);
        bool found_target = false;
        // TODO: look at this closer, and extract to function
        while (!found_target) {
            if (target_index == path->size() - 1) {
                if (!on_root){
                    if (select_pos == AtRoot){
                        root_index = match_root(start);
                    }
                    path = &(root_paths[root_index]);
                    target_index = 0;
                    on_root = true;
                }else{
                    // Bound target point to last point on root
                    target_point = (*path)[target_index];
                    found_target = true;
                    done = true;
                }
            } else if (travelled > distance_to_travel) {
                // Backtrack and travel exactly distance needed
                if(target_index != 0) target_index--;
                glm::vec3 last_step = (*path)[target_index + 1] - (*path)[target_index];
                travelled -= glm::length(last_step);
                float left_to_travel = distance_to_travel - travelled;
                target_point =
                    (*path)[target_index] + glm::normalize(last_step) * left_to_travel;
                found_target = true;
            } else {
                // Travel down the path
                travelled += glm::distance((*path)[target_index], (*path)[target_index + 1]);
                target_index++;
            }
        }
        // use target point to calculate canonical direction
        glm::vec3 canonical_direction = glm::normalize(target_point - last_closest);

        // Generate trials
        struct Trial {
            glm::vec3 head;
            float distance;
            float angle;
        };
        std::vector<Trial> trials;
        float max_trial_distance = 0.f;
        float min_trial_distance = FLT_MAX;
        float max_trial_angle = 0.f;
        for (int i = 0; i < num_trials; i++) {
            glm::vec3 trial_head = start + segment_length * 
                random_vector(canonical_direction, glm::radians(max_angle));
            float val = grid.get_in_pos(trial_head);
            if (val<=reject_iso) {
                float distance = glm::distance(trial_head, target_point);
                float angle = glm::angle(trial_head - start, canonical_direction);
                trials.push_back({trial_head, distance, angle});
                max_trial_distance = fmax(max_trial_distance, distance);
                min_trial_distance = fmin(min_trial_distance, distance);
                max_trial_angle = fmax(max_trial_angle, angle);
            }
        }

        // If no valid trials add strand up to this moment
        if (trials.empty()){ //TODO: could change this to back up
            strands_terminated++;
            break;
        }

        //  Evaluate trials
        int best_trial = 0;
        float best_fitness = 0.f;
        for (int i = 1; i < trials.size(); i++) {
            float distance_metric = 1 - (trials[i].distance - min_trial_distance) 
                                        / (max_trial_distance - min_trial_distance);
            float direction_metric = 1 - (trials[i].angle / max_trial_angle);
            float fitness = alpha * distance_metric + (1 - alpha) * direction_metric;
            if (fitness >= best_fitness) {
                best_trial = i;
                best_fitness = fitness;
            }
        }
        strand.push_back(trials[best_trial].head);
        closest_index = closest_node_on_path(strand.back(), *path, target_index, 5).first;
    }
    // Occupy strand path
    if (strand.size()<=2) return;
    strands.push_back(strand);
    grid.fill_path(strand, evalfunc, offset);
}

size_t Strands::match_root(glm::vec3 position){
    if (root_pool.empty()) {
        root_pool.resize(root_paths.size());
        std::iota(root_pool.begin(), root_pool.end(), 0);
    }
    size_t match_index = 0;
    if (select_method == AtRandom) {
        match_index = rng() % root_pool.size();
    } else if (select_method == WithAngle) {
        glm::vec3 angle_vec = position - tree.get_root_pos();
        angle_vec.y = 0.f;
        angle_vec = glm::normalize(angle_vec);
        float largest_cos = -1.f;
        for (size_t j = 0; j < root_pool.size(); j++) {
            float cos = glm::dot(angle_vec, root_vecs[root_pool[j]]);
            if (cos > largest_cos) {
                largest_cos = cos;
                match_index = j;
            }
        }
    }
    size_t match = root_pool[match_index];
    if (select_pool == NotSelected) {
        root_pool.erase(root_pool.begin() + match_index);
    }
    return match;
}

Mesh<Vertex> Strands::get_mesh() const {
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
    glm::vec3 blue(0,0,1);  // Placed Earlier
    glm::vec3 red(1,0,0);   // Placed Later
    int i = 0;
    for (auto path : strands) {
        glm::vec3 color = (1-((float)i/strands.size()))*blue+((float)i/strands.size())*red;
        size_t start_index = vertices.size();
        for (auto position : path) {
            vertices.push_back(Vertex{position, color});
        }
        size_t end_index = vertices.size();
        for (int i = start_index; i < end_index - 1; i++) {
            indices.push_back(i);
            indices.push_back(i + 1);
        }
        i++;
    }
    return Mesh(vertices, indices);
}

// Non-member helper functions
glm::vec3 random_vector(glm::vec3 axis, float angle) {
    const glm::vec3 x_axis(1, 0, 0);
    glm::quat rotation = glm::angleAxis(
            glm::angle(axis, x_axis), glm::normalize(glm::cross(x_axis, axis)));
    // CODE CITED
    // from https://community.khronos.org/t/random-vectors/41467/3 imported_jwatte
    std::uniform_real_distribution<float> x_rand(glm::cos(angle), 1);
    std::uniform_real_distribution<float> a_rand(-glm::pi<float>(),
    glm::pi<float>());
    float x = x_rand(rng);
    float a = a_rand(rng);
    float r = sqrtf(1 - x * x);
    float y = glm::sin(a) * r;
    float z = glm::cos(a) * r;
    return rotation * glm::vec3(x, y, z);
    // CODE CITED
}
