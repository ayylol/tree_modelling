#pragma once

#include <algorithm>
#include <chrono>
#include <nlohmann/json_fwd.hpp>
#include <random>
#include <utility>
#include <vector>
#include <optional>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/vector_angle.hpp>

#include "rendering/mesh.h"

#include "tree/grid.h"
#include "tree/implicit.h"
#include "tree/skeleton.h"

#include "util/geometry.h"

class Strands {
public:
    Strands(const Skeleton &tree, Grid &grid, Implicit& evalfunc, nlohmann::json options);
    Mesh<Vertex> get_mesh() const;
private:
    void add_strands(unsigned int amount);
    void add_strand(size_t shoot_index);
    size_t match_root(glm::vec3 pos);
    Implicit &evalfunc;
    const Skeleton& tree;
    std::vector<std::vector<glm::mat4>> shoot_frames;
    std::vector<std::vector<glm::mat4>> root_frames;
    std::vector<std::vector<glm::vec3>> strands;
    Grid &grid;

    // Strand Creation Helper Functions

    struct TargetResult{
        size_t index;
        glm::mat4 frame;
        float travelled;
    }; 
    struct Trial {
        glm::vec3 head;
        float distance;
        float angle;
    };
    TargetResult find_target(const std::vector<glm::mat4>& path, size_t start_index, float travel_dist);
    std::optional<glm::vec3> find_extension(glm::vec3 from, glm::mat4 frame_from, glm::mat4 frame_to);
    std::optional<glm::vec3> find_extension_fs(glm::vec3 from, glm::mat4 frame_from, glm::mat4 frame_to);
    std::optional<glm::vec3> find_extension_heading(glm::vec3 from, glm::mat4 frame);
    std::optional<glm::vec3> find_extension_canoniso(glm::vec3 from, glm::mat4 frame_from, glm::mat4 frame_to);
    TargetResult find_closest(glm::vec3 pos, const std::vector<glm::mat4>& path, size_t start_index, int overshoot);

    // Strand Creation Vars
    enum Method{
        CanonDir,
        LocalPosMatching,
        HeadingDir,
        CanonIso,
    } method;
    float segment_length;
    int num_trials;
    float max_angle;
    float local_spread;
    float reject_iso;
    float offset;
    // Eval Weights
    float iso_eval;
    float target_iso;
    float local_eval;
    float frame_eval;
    // Root Matching Vars
    enum SelectMethod{
        AtRandom,
        WithAngle,
    } select_method = WithAngle;
    enum SelectPos{
        AtLeaf,
        AtRoot,
    } select_pos = AtRoot;
    enum SelectPool{
        All,
        NotSelected,
    } select_pool = NotSelected;

    std::vector<size_t> root_pool;
    std::vector<glm::vec3> root_vecs;

    int strands_terminated = 0;
};
