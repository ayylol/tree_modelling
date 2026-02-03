#pragma once

#include <algorithm>
#include <chrono>
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
#include <nlohmann/json.hpp>

#include "util/geometry.h"

class Strands {
public:
    enum StrandType {
        Structure,
        Texture
    };
    Strands(const Skeleton &tree, Grid &grid, Grid &texture_grid, nlohmann::json options);
    Mesh<Vertex> get_mesh(float start = 0.0f, float end = 1.0f, StrandType type = Structure) const;
    Mesh<Vertex> visualize_node(float strand, float node) const;
    Mesh<Vertex> visualize_searchpoint(float strand) const;
    Mesh<Vertex> visualize_keypoints(float strand) const;
    void add_strands(unsigned int amount);
    int add_stage();
private:
    void add_strand(size_t shoot_index, int age, StrandType type = Structure);
    size_t match_root(glm::vec3 pos, glm::mat4 frame);
    std::pair<size_t,size_t> match_root_all(glm::vec3 pos);
    //Implicit &evalfunc;
    const Skeleton& tree;
    std::vector<std::vector<glm::mat4>> shoot_frames;
    std::vector<std::vector<glm::mat4>> root_frames;
    std::vector<std::vector<glm::vec3>> strands;
    std::vector<std::pair<size_t,size_t>> inflection_points;
    std::vector<std::vector<glm::vec3>> texture_strands;
    std::vector<std::vector<std::vector<glm::vec3>>> node_info;

    struct Keypoints {
      glm::vec3 la_start;
      glm::vec3 la_peak;
      glm::vec3 base_node;
      glm::vec3 transition_node;
      glm::vec3 enter_root_node;
    };
    std::vector<Keypoints> keypoints;

    Grid &grid;
    Grid &texture_grid;

    // Strand Creation Helper Functions
    std::vector<glm::vec3> smooth(
            const std::vector<glm::vec3>& in, 
            int times, float peak_influence, float min_influence, 
            int start, int peak, int end);
    glm::vec3 move_extension(glm::vec3 head, glm::vec3 close, float iso);

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

    int num_strands;
    int stages_left;
    int strands_per_stage;
    //
    int longest_shoot_length=0;
    //
    // Node observed
    float node_observed=0.0;
    size_t strand_observed=0;
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
    // Experimental
    int start_node=50;
    float bsearch_iso;
    float _interp;
    float _interp_bias;
    // Lookahead Vars
    float strand_lookahead_max;
    float lookahead_factor_min = 1.0f;
    float lookahead_factor_max = 2.0f;
    float laf_step;
    int la_interp_start=20;
    int la_interp_peak=10;
    float la_red_max=10.f;
    float la_red_min=1.f;
    //transition zone
    float searchpoint_step = 0.1f;
    float bias_step = 0.1f;
    // Biasing
    float bias_amount = 1.0f;
    // Smoothing vars
    int sm_iter = 100;
    float sm_min = 0.001;
    float sm_peak = 0.15;
    float sm_start = 0.9;
    float sm_end = 0.7;
    //
    float max_val = 3.0f;
    float leaf_min_range;
    float base_max_range;
    float root_min_range;
    float root_angle_node;
    // Texture Strand Vars ///////
    int tex_chance_start = 0;
    float tex_max_chance = 1.0f;
    float tex_chance = 0.0f;
    float tex_max_val = 30.f;
    float tex_max_range = 0.013f;
    float tex_shoot_range = 0.001;
    float tex_root_range = 0.f;
    float tex_chance_step;
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
    } select_pool = All;

    std::vector<size_t> root_pool;
    std::vector<glm::vec3> root_vecs;

    int strands_terminated = 0;
};
