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
    enum StrandType {
        Structure,
        Texture
    };
    Strands(const Skeleton &tree, Grid &grid, Grid &texture_grid, nlohmann::json options);
    Mesh<Vertex> get_mesh(float start = 0.0f, float end = 1.0f, StrandType type = Structure) const;
private:
    void add_strands(unsigned int amount);
    void add_strand(size_t shoot_index, int age, StrandType type = Structure);
    size_t match_root(glm::vec3 pos);
    //Implicit &evalfunc;
    const Skeleton& tree;
    std::vector<std::vector<glm::mat4>> shoot_frames;
    std::vector<std::vector<glm::mat4>> root_frames;
    std::vector<std::vector<glm::vec3>> strands;
    std::vector<std::vector<glm::vec3>> texture_strands;
    Grid &grid;
    Grid &texture_grid;

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
    std::optional<glm::vec3> find_extension(glm::vec3 from, glm::mat4 frame_from, glm::mat4 frame_to, bool bias=false);
    std::optional<glm::vec3> find_extension_fs(glm::vec3 from, glm::mat4 frame_from, glm::mat4 frame_to);
    std::optional<glm::vec3> find_extension_heading(glm::vec3 from, glm::mat4 frame);
    std::optional<glm::vec3> find_extension_canoniso(glm::vec3 from, glm::mat4 frame_from, glm::mat4 frame_to, bool bias=false, float bias_amount = 1.0f);
    std::optional<glm::vec3> find_extension_ptfiso(glm::vec3 from, glm::mat4 frame_from, glm::mat4 frame_to);
    std::optional<glm::vec3> find_extension_canonptfeval(glm::vec3 from, glm::mat4 frame_from, glm::mat4 frame_to);
    std::optional<glm::vec3> find_extension_ptfcanoneval(glm::vec3 from, glm::mat4 frame_from, glm::mat4 frame_to);
    std::optional<glm::vec3> find_extension_texture(glm::vec3 from, glm::mat4 frame_from, glm::mat4 frame_to);
    //TargetResult find_closest(glm::vec3 pos, const std::vector<glm::mat4>& path, size_t start_index, int overshoot);
    TargetResult find_closest(glm::vec3 pos, const std::vector<glm::mat4>& path, int start_index, int end_index);

    // Strand Creation Vars
    enum Method{
        CanonDir,
        LocalPosMatching,
        HeadingDir,
        CanonIso,
        PTFIso,
        CanonPTFEval,
        PTFCanonEval,
        TextureExt
    } method = CanonDir;
    float segment_length;
    int num_trials;
    float max_angle;
    float local_spread;
    float reject_iso;
    float offset;
    float lookahead_factor;
    float lookahead_factor_current;
    float lookahead_factor_min = 1.0f;
    float lookahead_factor_max = 2.0f;
    float max_val = 3.0f;
    float leaf_min_range;
    float base_max_range;
    float root_min_range;
    float root_angle_node;
    float bias_amount = 1.0f;
    // Texture Strand Vars ///////
    int tex_chance_start = 0;
    float tex_max_chance = 1.0f;
    float tex_chance = 0.0f;
    float tex_max_val = 30.f;
    float tex_max_range = 0.013f;
    float tex_shoot_range = 0.001;
    float tex_root_range = 0.f;
    /////////////////////////////
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
        AtLeastOnce,
    } select_pool = AtLeastOnce;

    std::vector<size_t> root_pool;
    std::vector<glm::vec3> root_vecs;

    int strands_terminated = 0;
};
