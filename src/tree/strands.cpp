#include "glm/common.hpp"
#include "glm/ext/scalar_constants.hpp"
#include "glm/fwd.hpp"
#include "glm/geometric.hpp"
#include "glm/gtx/norm.hpp"
#include "glm/gtx/vector_angle.hpp"
#include "glm/vector_relational.hpp"
#include "strands.h"
#include <algorithm>
#include <cstdlib>
#include <glm/gtx/io.hpp>
#include <omp.h>
#include <ostream>

std::vector<glm::vec3> Strands::smooth(const std::vector<glm::vec3> &in,
                                       int times, float peak_influence,
                                       float min_influence, int start, int peak,
                                       int end) {
  size_t n = in.size();
  start = std::max(1, start);
  end = std::min((int)n - 2, end);
  peak = std::clamp(peak, start, end);
  std::vector<glm::vec3> old = in;
  std::vector<glm::vec3> smoothed = in;
  while (times--) {
    float influence = min_influence;
    float i_inc = (peak_influence - min_influence) / (peak - start);
    float i_dec = (peak_influence - min_influence) / (peak - end);
    for (int i = 1; i < n - 2; ++i) {
      if (i >= start && i < peak) {
        influence += i_inc;
      } else if (i >= peak && i < end) {
        influence += i_dec;
      }
      smoothed[i] = influence * old[i - 1] + (1.f - 2 * influence) * old[i] +
                    influence * old[i + 1];
    }
    /*
     * TODO: REENABLE
    for (auto it=smoothed.begin()+1; it!=(smoothed.end()-1);++it){
        float d = glm::distance(*it,*(it-1))+glm::distance(*it,*(it+1));
        if (d<segment_length*1){
            smoothed.erase(it);
            it--;
        }
    }
    */
    old = smoothed;
  }
  return smoothed;
}

std::default_random_engine
    rng(std::chrono::system_clock::now().time_since_epoch().count());

glm::vec3 random_vector(glm::vec3 axis, float angle);
glm::vec2 random_vec2();
float lookahead_frame(float dist);

Strands::Strands(const Skeleton &tree, Grid &grid, Grid &texture_grid,
                 nlohmann::json options)
    : grid(grid), texture_grid(texture_grid),
      // evalfunc(evalfunc),
      tree(tree) {
  // Set up root & shoot paths
  for (size_t i = 0; i < tree.leafs_size(); i++) {
    shoot_frames.push_back(tree.get_strand(i, Skeleton::LEAF));
  }
  for (size_t i = 0; i < tree.roots_size(); i++) {
    std::vector<glm::mat4> root_frame = tree.get_strand(i, Skeleton::ROOT);
    // Reverse the path
    for (size_t i = 0; i < root_frame.size() / 2; i++) {
      size_t j = root_frame.size() - 1 - i;
      std::swap(root_frame[i], root_frame[j]);
    }
    root_frames.push_back(root_frame);
  }
  // Set strand options
  auto strand_options = options.at("strands");
  int num_strands = tree.leafs_size();
  if (strand_options.contains("num_abs")) {
    num_strands = strand_options.at("num_abs");
  } else if (strand_options.contains("num_per")) {
    num_strands = (int)(num_strands * (float)strand_options.at("num_per"));
  }
  if (strand_options.contains("sectorality")) {
    select_method = strand_options.at("sectorality") ? WithAngle : AtRandom;
  }
  max_val = strand_options.at("max_val");
  segment_length = strand_options.at("segment_length");
  num_trials = strand_options.at("num_trials");
  max_angle = strand_options.at("max_angle");
  // LA
  lookahead_factor_max = strand_options.at("lookahead_max");
  lookahead_factor_min = strand_options.at("lookahead_min");
  la_interp_start =
      std::clamp((float)strand_options.at("la_interp_start"), 0.f, 1.f);
  la_interp_peak =
      std::clamp((float)strand_options.at("la_interp_peak"), 0.f, 1.f);
  la_red_max = strand_options.at("la_red_max");
  la_red_min = strand_options.at("la_red_min");
  // Smooth
  sm_iter = strand_options.at("sm_iter");
  sm_min = strand_options.at("sm_min");
  sm_peak = strand_options.at("sm_peak");
  sm_start = std::clamp((float)strand_options.at("sm_start"), 0.f, 1.f);
  sm_end = std::clamp((float)strand_options.at("sm_end"), 0.f, 1.f);
  //
  reject_iso = strand_options.at("reject_iso");
  target_iso = strand_options.at("target_iso");
  iso_eval = strand_options.at("iso_eval");
  local_eval = strand_options.at("local_eval");
  frame_eval = strand_options.at("frame_eval");
  float total_eval_weight = iso_eval + local_eval + frame_eval;
  iso_eval /= total_eval_weight;
  local_eval /= total_eval_weight;
  frame_eval /= total_eval_weight;
  local_spread = strand_options.at("local_spread");
  bias_amount = strand_options.at("bias_amount");
  // Implicit Vals
  leaf_min_range = strand_options.at("leaf_min_range");
  base_max_range = strand_options.at("base_max_range");
  root_min_range = strand_options.at("root_min_range");
  // Initialize Root Angle Vectors
  root_angle_node =
      std::clamp((float)strand_options.at("root_angle_node"), 0.05f, 1.f);
  root_vecs.reserve(root_frames.size());
  for (size_t i = 0; i < root_frames.size(); i++) {
    // glm::vec3 angle_vec =
    // frame_position(root_frames[i][(int)((root_frames[i].size()-1)*root_angle_node)])
    // - tree.get_root_pos();
    /*
    glm::vec3 angle_vec =
        frame_position(
            root_frames[i][std::min(60, (int)root_frames[i].size() - 1)]) -
        tree.get_root_pos();
        */
    /*
    glm::vec3 angle_vec =
        frame_position(
            root_frames[i][root_frames[i].size() - 1]) - tree.get_root_pos();
            */
    glm::vec3 angle_vec =
        frame_position(
            root_frames[i][std::max(20,(int)root_frames[i].size() - 100)]) -
        tree.get_root_pos();
    angle_vec.y = 0.f;
    angle_vec = glm::normalize(angle_vec);
    root_vecs.push_back(angle_vec);
  }
  // Texture vars
  if (strand_options.contains("texture")) {
    auto tex_options = strand_options.at("texture");
    tex_max_val = tex_options.at("max_val");
    tex_max_range = tex_options.at("base_max_range");
    tex_shoot_range = tex_options.at("leaf_min_range");
    tex_root_range = tex_options.at("root_min_range");
    tex_chance_start = (float)tex_options.at("chance_start") * num_strands;
    tex_max_chance = tex_options.at("chance_max");
  }
  add_strands(num_strands);
}

Mesh<Vertex> Strands::visualize_node(float strand, float node) const {
  glm::vec3 col1(0, 1, 0);
  glm::vec3 col2(1, 0, 1);
  strand = std::clamp(strand, 0.0f, 1.f);
  node = std::clamp(node, 0.0f, 1.f);
  size_t strand_i = strand * (strands.size() - 1);
  size_t node_i = node * (strands[strand_i].size() - 1);
  std::vector<Vertex> vertices;
  vertices.push_back(Vertex(strands[strand_i][node_i], col2));
  for (auto node : node_info[strand_i][node_i]) {
    vertices.push_back(Vertex(node, col2));
  }
  std::vector<GLuint> indices;
  if (node_info[strand_i][node_i].size() == 4) {
    indices = {0, 2, 1, 3, 4};
  } else {
    indices = {0, 1, 2};
  }
  return Mesh(vertices, indices);
}
Mesh<Vertex> Strands::get_mesh(float start, float end, StrandType type) const {
  start = std::max(0.0f, start);
  end = std::clamp(end, start, 1.f);
  std::vector<Vertex> vertices;
  std::vector<GLuint> indices;
  glm::vec3 red(1, 0, 0); // Placed Later
  glm::vec3 green(0, 1, 0);
  glm::vec3 blue(0, 0, 1); // Placed Earlier
  // float start = 0.;
  // float end = 1.0;
  auto &strand_list = type == Structure ? strands : texture_strands;
  if (type == Texture) {
    blue = glm::vec3(0, 1, 0);
    red = glm::vec3(0, 1, 0);
  }
  if (strand_list.size() != 0) {
    for (int i = start * (strand_list.size() - 1);
         i <= end * (strand_list.size() - 1); i++) {
      auto path = strand_list[i];
      float percent = ((float)i / strand_list.size() - start) / (end - start);
      // glm::vec3 color = (1-percent)*blue+(percent)*red;
      size_t start_index = vertices.size();
      int j = 0;
      for (auto position : path) {
        glm::vec3 color = j < inflection_points[i].first    ? blue
                          : j < inflection_points[i].second ? red
                                                            : blue;
        vertices.push_back(Vertex{position, color});
        j++;
      }
      size_t end_index = vertices.size();
      for (int i = start_index; i < end_index - 1; i++) {
        indices.push_back(i);
        indices.push_back(i + 1);
      }
    }
  }
  return Mesh(vertices, indices);
}

void Strands::add_strands(unsigned int amount) {
  std::vector<size_t> paths(shoot_frames.size());
  std::iota(paths.begin(), paths.end(), 0);
  std::shuffle(paths.begin(), paths.end(), rng);
  float lhf_step = (lookahead_factor_max-lookahead_factor)/(amount);
  //float lhf_step = (lookahead_factor_max - lookahead_factor) / (paths.size());
  float texture_chance_step = tex_max_chance / (amount - tex_chance_start);
  lookahead_factor_current = lookahead_factor;
  for (size_t i = 0; i < amount; i++) {
    std::cout << "\rStrand: " << i + 1 << "/" << amount;
    std::flush(std::cout);
    add_strand(paths[i % paths.size()], i);
    if (i > tex_chance_start)
      tex_chance += texture_chance_step;
    lookahead_factor_current += lhf_step;
  }
  std::cout << std::endl;
  std::cout << std::endl;
  // std::cout << "Strands Termniated: "<< strands_terminated << std::endl;
}

// THE ALGORITHM THAT IMPLEMENTS STRAND VOXEL AUTOMATA
void Strands::add_strand(size_t shoot_index, int age, StrandType type) {
  if (shoot_index >= shoot_frames.size())
    return;
  // Set up strand
  const std::vector<glm::mat4> *shoot_path = &(shoot_frames[shoot_index]);
  const std::vector<glm::mat4> *root_path = nullptr;
  const std::vector<glm::mat4> *path = shoot_path;
  // size_t closest_index =
  // std::clamp((int)shoot_path->size()-start_node,0,(int)shoot_path->size()-50);
  //  Find start index
  size_t closest_index;
  size_t a = 0, b = shoot_path->size() - 20;
  int root_nodes=0;
  while (b - a > 5) {
    closest_index = a + (b - a) / 2;
    if (grid.eval_pos(frame_position((*path)[closest_index])) <= 0.01f) {
      a = closest_index;
    } else {
      b = closest_index;
    }
  }
  if (closest_index > 20)
    closest_index -= 20;
  closest_index =
      std::clamp(closest_index, (size_t)0, (size_t)shoot_path->size() - 20);
  //
  glm::mat4 last_closest = (*path)[closest_index];
  std::vector<glm::vec3> strand{frame_position(last_closest)};

  // SETUP AUXILARY INFO
  inflection_points.push_back({0, 0});
  node_info.push_back({});
  node_info.back().push_back({});
  node_info.back().back().push_back(frame_position(last_closest));
  // SETUP AUXILARY INFO

  // Set up lookahead value
  // lookahead_factor=lookahead_factor_current; // CHANGE THIS
  const int la_start_node = la_interp_start * (shoot_path->size() - 1);
  const int la_peak_node = la_interp_peak * (shoot_path->size() - 1);
  // lookahead_factor=1.0f;
  //  Set up lookahead root reduction
  //  Loop until on root, and target node is the end
  bool on_root = false;
  bool target_on_root = false;
  bool done = false;
  size_t inflection = 0;
  switch (type) {
  case Structure:
    method = CanonDir;
    // method=LocalPosMatching;
    break;
  case Texture:
    method = TextureExt;
    // method=PTFIso;
    // method=LocalPosMatching;
    break;
  }
  int transition_node = 0;
  int num_extensions;

  while (!done) {
    if (on_root) {
      num_extensions--;
      if (num_extensions == 0)
        done = true;
    }
    // Calculate lookahead factor
    float la_interp = on_root                          ? 0.0f
                      : closest_index <= la_start_node ? 0.0f
                      : closest_index <= la_peak_node
                          ? (float)(closest_index - la_start_node) /
                                (la_peak_node - la_start_node)
                          : 1.f;
    lookahead_factor = (1 - la_interp) * lookahead_factor_min +
                       la_interp * lookahead_factor_current;
    // Start of this segment is head of last
    glm::vec3 start(strand[strand.size() - 1]);
    float la_max = 10.0f;
    float distance_to_travel = std::min(
        lookahead_factor * (segment_length +
                            glm::distance(frame_position(last_closest), start)),
        la_max);

    // Find target
    TargetResult target = find_target(*path, closest_index, distance_to_travel);
    if (!on_root && target_on_root && target.index != path->size()-1){
      // Target is in transition area but the target here crept back onto shoot
      // so we nudge it back into the root so things dont crash
      target.index = 0;
      target.frame = (*root_path)[0];
    }
    if (target.index == path->size() - 1) {
      if (!on_root) { // switch path
        if (!target_on_root) {
          inflection_points[inflection_points.size() - 1].first =
              strand.size() - 1;
          target_on_root = true;
          if (root_path == nullptr) {
            root_path = &(root_frames[match_root(strand[strand.size() - 1],
                                                 (*path)[closest_index])]);
            transition_node = closest_index;
          }
        }
        float alpha = (float)closest_index / shoot_path->size();
        float reduction = (1 - alpha) * la_red_max + alpha * la_red_min;
        target =
            find_target(*root_path, 0,
                        (distance_to_travel - target.travelled) / reduction);
      } else {
        // done = true;
      }
    }
    // ADD TARGET to visualization
    node_info.back().back().push_back(frame_position(target.frame));
    // end of find target section

    // Add extension
    std::optional<glm::vec3> ext;
    if (target_on_root || on_root)
      ext = find_extension(strand.back(), last_closest, target.frame, true);
    else
      ext = find_extension(strand.back(), last_closest, target.frame);

    if (!ext) {
      ext = find_extension_canoniso(strand.back(), last_closest, target.frame,
                                    false);
    }
    strand.push_back(ext.value());
    node_info.back().push_back({});

    TargetResult next;
    if (!on_root && target_on_root) {
      TargetResult shoot_closest =
          find_closest(strand.back(), *shoot_path, closest_index + 1,
                       shoot_path->size() - 1);
      TargetResult root_closest =
          find_closest(strand.back(), *root_path, 0, root_path->size() - 1);
      if (shoot_closest.travelled <
          root_closest.travelled) { // note travelled here is dist2 between
                                    // closest and node
        next = shoot_closest;
      } else {
        //rematch root
        /*
        root_path = &(root_frames[match_root(strand[strand.size() - 1],
                                             (*path)[closest_index])]);
                                             */
        next = root_closest;
        path = root_path;
        on_root = true;
        inflection = strand.size() - 1;
        lookahead_factor = 1.0f;
        num_extensions = strand.size();
        inflection_points[inflection_points.size() - 1].second =
            strand.size() - 1;
      }
    } else {
      next = find_closest(strand.back(), *path, closest_index + 1,
                          std::max(target.index, closest_index + 1));
    }

    // Binary search for final extension
    if (target_on_root) {
      _interp += 0.1f;
      _interp_bias += 0.1f; // Parameter
      _interp = std::min(_interp, 1.f);
      _interp_bias = std::min(_interp_bias, 1.f);
      TargetResult root_closest =
          find_closest(strand.back(), *root_path, 0, root_path->size() - 1);
      float alpha =
          std::min(root_closest.index / (root_path->size() * 0.5f), 1.f);
      bsearch_iso = reject_iso * (1.f - alpha) + reject_iso * alpha;
      // bsearch_iso=reject_iso*(1.f-alpha)+(reject_iso-10)*alpha;
    } else {
      _interp = 0.f;
      _interp_bias = 0.f;
      bsearch_iso = reject_iso;
    }
    node_info.back().back().push_back(frame_position(next.frame));
    if (target_on_root && !on_root) {
      TargetResult root_closest =
          find_closest(strand.back(), *root_path, 0, target.index);
      float alpha = _interp;
      glm::vec3 bin_point = frame_position(root_closest.frame) * alpha +
                            frame_position(next.frame) * (1.f - alpha);
      strand[strand.size() - 1] =
          move_extension(strand.back(), bin_point, bsearch_iso);
      node_info.back().back().push_back(bin_point);
      node_info.back().back().push_back(frame_position(root_closest.frame));
    } else {
      strand[strand.size() - 1] = move_extension(
          strand.back(), frame_position(next.frame), bsearch_iso);
    }
    /// end of binary search

    if (next.index >= path->size() - 1 && on_root) {
      done = true;
    }
    if (on_root && root_nodes>20 && root_nodes%5==0){
      auto p = match_root_all(strand.back());
      root_path = &(root_frames[p.first]);
      path = root_path;
      closest_index = std::max((size_t)0,p.second-1);
      last_closest = (*path)[closest_index];
    }else{
      closest_index = next.index;
      last_closest = next.frame;
    }
    if (on_root) root_nodes++;
  }
  // Occupy strand path
  if (strand.size() <= 2)
    return;
  float r = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
  switch (type) {
  case Structure:
    // Smooth
    strand = smooth(strand, sm_iter, sm_peak, sm_min, inflection * sm_start,
                    inflection,
                    inflection + ((strand.size() - inflection - 1) * sm_end));
    strands.push_back(strand);
    assert(strand.size() == node_info.back().size());
    assert(strands.size() == node_info.size());
    if (r < tex_chance) {
      texture_strands.push_back(strand);
      texture_grid.fill_path(strands.size(), strand, tex_max_val, tex_max_range,
                             tex_shoot_range, tex_root_range, inflection);
    }
    grid.fill_path(strands.size(), strand, max_val, base_max_range,
                   leaf_min_range, root_min_range, inflection);
    break;
  case Texture:
    // FIXME: CHANGE MARKER
    texture_strands.push_back(strand);
    // grid.fill_path(texture_strands.size(), strand, tex_max_val,
    // tex_max_range, tex_shoot_range, tex_root_range, inflection);
    // texture_grid.fill_path(texture_strands.size(), strand, tex_max_val,
    // tex_max_range, tex_shoot_range, tex_root_range, inflection);
    texture_grid.fill_path(texture_strands.size(), strand, tex_max_val,
                           tex_max_range, tex_shoot_range, tex_root_range,
                           inflection);
    // texture_grid.fill_path(0,strand, 5.0, 0.008, 0.001, 0.0001, inflection);
    break;
  }
}

// Strand creation helper functions
Strands::TargetResult Strands::find_target(const std::vector<glm::mat4> &path,
                                           size_t start_index,
                                           float travel_dist) {
  TargetResult result = {start_index, path[start_index], 0.f};
  glm::vec3 target_point = frame_position(path[result.index]);
  // glm::distance2(frame_position(path[start_index]),frame_position(result.frame))<=std::pow(segment_length,2)
  while (result.travelled < travel_dist && result.index != path.size() - 1) {
    result.travelled += glm::distance(frame_position(path[result.index]),
                                      frame_position(path[result.index + 1]));
    result.index++;
    result.frame = path[result.index];
  }
  // Travelled further than allowed
  if (result.index == path.size() - 1) {
    result.frame = path[result.index];
  } else if (result.travelled > travel_dist && result.index != 0) {
    result.index--;
    glm::vec3 p1 = frame_position(path[result.index]);
    glm::vec3 p2 = frame_position(path[result.index + 1]);
    result.frame = path[result.index];
    glm::vec3 last_step = p2 - p1;
    float left_to_travel = result.travelled - travel_dist;
    result.frame[3] =
        glm::vec4(p1 + glm::normalize(last_step) * left_to_travel, 1.f);
    result.travelled = travel_dist;
  }
  return result;
}

std::optional<glm::vec3> Strands::find_extension(glm::vec3 from,
                                                 glm::mat4 frame_from,
                                                 glm::mat4 frame_to,
                                                 bool bias) {
  glm::vec3 target_point = frame_position(frame_to);
  glm::vec3 canonical_direction =
      glm::normalize(target_point - frame_position(frame_from));
  // Generate trials
  struct Trial {
    glm::vec3 head;
    float distance;
    float angle;
    float val;
  };
  std::vector<Trial> trials;
  trials.reserve(num_trials);
// TODO: Parallelize this
#pragma omp parallel for
  for (int i = 0; i < num_trials; i++) {
    glm::vec3 r_vec = random_vector(canonical_direction, glm::radians(max_angle));
    glm::vec3 trial_head = from + segment_length * r_vec;
    assert(!glm::any(glm::isnan(from)));
    assert(!glm::any(glm::isnan(r_vec)));
    assert(!glm::any(glm::isnan(trial_head)));
    float val = grid.eval_pos(trial_head);
    // float val = grid.lazy_in_check(grid.pos_to_grid(trial_head),reject_iso);
    if (val <= reject_iso) {
      float distance;
      if (bias) {
        float current_bias = (1 - _interp_bias) + bias_amount * _interp_bias;
        glm::vec3 biased_head = trial_head;
        biased_head.y /= current_bias;
        glm::vec3 biased_point = target_point;
        biased_point.y /= current_bias;
        distance = glm::distance(biased_head, biased_point);
      } else {
        distance = glm::distance(trial_head, target_point);
      }
      float angle = glm::angle(trial_head - from, canonical_direction);
      trials.push_back({trial_head, distance, angle, val});
    }
  }
  //
  // If no valid trials add strand up to this moment
  if (trials.empty())
    return {};
  //  Evaluate trials
  int best_trial = 0;
  float best_fitness = FLT_MAX;
  for (int i = 1; i < trials.size(); i++) {
    if (trials[i].distance <= best_fitness) {
      best_trial = i;
      best_fitness = trials[i].distance;
    }
  }
  assert(!glm::any(glm::isnan(trials[best_trial].head)));
  return trials[best_trial].head;
}

// Local frame based sample finding
std::optional<glm::vec3> Strands::find_extension_fs(glm::vec3 from,
                                                    glm::mat4 frame_from,
                                                    glm::mat4 frame_to) {
  assert(false);
  return {};
  /*
    glm::mat4 inv_from = frame_inverse(frame_from);
    glm::mat4 inv_to = frame_inverse(frame_to);
    // Calculate Position in local frame
    glm::vec3 local_pos = inv_from*glm::vec4(from,1.f);
    local_pos.y=0.f;

    // Init Eval Bounds
    float min_val_diff = FLT_MAX;
    float max_local_dist2 = 0.f;
    float min_local_dist2 = FLT_MAX;
    float max_frame_dist2 = 0.f;
    float min_frame_dist2 = FLT_MAX;

    // Generate samples
    struct Trial {
        glm::vec3 local;
        glm::vec3 global;
        float val;
    };
    std::vector<Trial> trials;
    trials.reserve(num_trials);
    #pragma omp parallel for
    for (int i = 0; i < num_trials; i++) {
        glm::vec2 sample = local_spread*random_vec2();
        glm::vec3 local_sample = local_pos;
        local_sample.x+=sample.x;
        local_sample.z+=sample.y;
        glm::vec3 global_sample = frame_to*glm::vec4(local_sample,1.f);
        float val = grid.eval_pos(global_sample);
        if (val<reject_iso){
            trials.push_back({local_sample,global_sample, val});
            // Update Evaluation Bounds
            max_val_diff = std::max(max_val_diff,std::abs(val-target_iso));
            min_val_diff = std::min(min_val_diff,std::abs(val-target_iso));
            max_local_dist2 =
    std::max(max_local_dist2,glm::distance2(local_sample,local_pos));
            min_local_dist2 =
    std::min(min_local_dist2,glm::distance2(local_sample,local_pos));
            max_frame_dist2 =
    std::max(max_frame_dist2,glm::length2(local_sample)); min_frame_dist2 =
    std::min(min_frame_dist2,glm::length2(local_sample));
        }
    }
    if(trials.empty()) return {};

    Trial best_trial = trials[0];
    float best_fitness = -1.f;
    for (auto trial : trials){
        float iso_metric=max_val_diff != min_val_diff ?
    iso_eval*(max_val_diff-(std::abs(trial.val-target_iso))/(max_val_diff-min_val_diff))
    : 0.f; float
    local_metric=local_eval*((max_local_dist2-glm::distance2(trial.local,local_pos))/(max_local_dist2-min_local_dist2));
        float
    frame_metric=frame_eval*((max_frame_dist2-glm::length2(trial.local))/(max_frame_dist2-min_frame_dist2));
        float fitness = iso_metric+local_metric+frame_metric;
        if (best_fitness <= fitness){
            best_fitness = fitness;
            best_trial = trial;
        }
    }

    // return best sample
    glm::vec3 extension =
    from+segment_length*glm::normalize(best_trial.global-from); return
    extension;
    //return best_trial.global;
    */
}
std::optional<glm::vec3> Strands::find_extension_heading(glm::vec3 from,
                                                         glm::mat4 frame) {
  glm::vec3 target_extension = frame * glm::vec4(0, -segment_length, 0, 1);
  glm::vec3 extension =
      from - (glm::mat3(frame) * glm::vec3(0, segment_length, 0));
  // Step until gradient is found
  int num_steps = 0;
  int max_steps = 10;
  glm::vec3 step = 0.1f * (target_extension - extension);
  while (glm::all(glm::lessThan(glm::abs(grid.eval_gradient(extension)),
                                glm::vec3(0.00001f))) &&
         num_steps <= max_steps) {
    extension += step;
    num_steps++;
  }
  num_steps = 0;
  max_steps = 20;
  // Step along gradient
  while (!glm::all(glm::lessThan(glm::abs(grid.eval_gradient(extension)),
                                 glm::vec3(0.00001f))) &&
         std::abs(grid.eval_pos(extension) - reject_iso) >= 0.4 &&
         num_steps <= max_steps) {
    glm::vec3 step =
        (grid.eval_pos(extension) - reject_iso) * grid.eval_gradient(extension);
    extension += step;
    num_steps++;
  }
  extension = from + segment_length * glm::normalize(extension - from);
  return extension;
}

std::optional<glm::vec3> Strands::find_extension_canoniso(glm::vec3 from,
                                                          glm::mat4 frame_from,
                                                          glm::mat4 frame_to,
                                                          bool bias,
                                                          float bias_amount) {
  glm::vec3 closest_pos = frame_position(frame_from);
  glm::vec3 target_extension = frame_position(frame_to);
  glm::vec3 extension =
      from + segment_length * glm::normalize(frame_position(frame_to) -
                                             frame_position(frame_from));
  // Step until gradient is found
  // Bias towards being on top of root
  // NOTE: Experimental
  if (bias) {
    glm::vec3 offset_no_y = target_extension - extension;
    offset_no_y.y = 0.f;
    // std::cout << (from-closest_pos).y <<std::endl;
    // std::uniform_real_distribution<float> rand_offset(0.2f, 0.8f);
    // extension = extension+rand_offset(rng)*offset_no_y;
    // extension = extension + offset_no_y;
    // extension = extension +
    // std::min(std::abs((from-closest_pos).y)/0.02f,1.0f)*offset_no_y;
    // extension = extension +
    // std::min(std::abs((from-closest_pos).y)/0.03f,std::abs(1-glm::dot(glm::vec3(frame_from*glm::vec4(0,1,0,0)),
    // glm::vec3(0,1,0))))*offset_no_y; extension = extension +
    // std::min(std::abs((from-closest_pos).y)/0.03f,std::abs(glm::dot(glm::vec3(frame_from*glm::vec4(0,1,0,0)),
    // glm::vec3(0,1,0))))*offset_no_y;
    extension = extension + bias_amount * offset_no_y;
    // std::cout<<std::abs(glm::dot(frame_from*glm::vec4(0,1,0,0),glm::vec4(0,1,0,0)))<<std::endl;
    // extension =
    // extension+(1.0f-std::abs(glm::dot(frame_from*glm::vec4(0,1,0,0),glm::vec4(0,1,0,0))))*offset_no_y;
  }
  //
  int num_steps = 0;
  int max_steps = 50;
  glm::vec3 step = 0.02f * (target_extension - extension);
  // while (glm::all(glm::isnan(grid.eval_gradient(extension))) &&
  // num_steps<=max_steps){
  while (glm::all(glm::lessThan(glm::abs(grid.eval_gradient(extension)),
                                glm::vec3(0.0001f))) &&
         num_steps <= max_steps) {
    extension += step;
    num_steps++;
  }
  // NOTE: EXPERIMENTAL
  /*
  if (bias){
      glm::vec3 offset_no_y = target_extension - extension;
      offset_no_y.y = 0.f;
      //std::cout << (from-closest_pos).y <<std::endl;
      std::uniform_real_distribution<float> rand_offset(0.2f, 0.6f);
      //extension = extension+rand_offset(rng)*offset_no_y;
      //extension = extension + offset_no_y;
      //extension = extension +
  std::min(std::abs((from-closest_pos).y)/0.03f,1.0f)*offset_no_y; extension =
  extension +
  std::min(std::abs((from-closest_pos).y)/0.02f,rand_offset(rng))*offset_no_y;
  }
  */
  // Step along gradient
  num_steps = 0;
  max_steps = 100;
  while (!glm::all(glm::isnan(grid.eval_gradient(extension))) &&
         std::abs(grid.eval_pos(extension) - reject_iso) >= 0.1 &&
         num_steps <= max_steps) {
    glm::vec3 step = 0.001f * (grid.eval_pos(extension) - reject_iso) *
                     grid.eval_gradient(extension);
    extension += step;
    // extension = from+segment_length*glm::normalize(extension-from);
    num_steps++;
  }
  extension = from + segment_length * glm::normalize(extension - from);
  return extension;
}

std::optional<glm::vec3> Strands::find_extension_ptfiso(glm::vec3 from,
                                                        glm::mat4 frame_from,
                                                        glm::mat4 frame_to) {
  glm::mat4 inv_from = frame_inverse(frame_from);
  glm::mat4 inv_to = frame_inverse(frame_to);
  // Calculate Position in local frame
  glm::vec3 local_pos = inv_from * glm::vec4(from, 1.f);
  local_pos.y = 0.f;
  glm::vec3 extension_local = local_pos;
  glm::vec3 extension = frame_to * glm::vec4(extension_local, 1.f);
  int num_steps = 0;
  int max_steps = 20;
  while (glm::all(glm::isnan(grid.get_norm_pos(extension))) &&
         num_steps <= max_steps) {
    extension_local -= (1.f / max_steps) * local_pos;
    extension = frame_to * glm::vec4(extension_local, 1.f);
    num_steps++;
  }
  glm::vec3 direction = extension_local;
  if (glm::length2(extension_local) < 0.000001f) {
    glm::vec2 rand_dir = random_vec2();
    direction.x = rand_dir.x;
    direction.z = rand_dir.y;
  }
  direction = 0.0001f * glm::normalize(direction);
  num_steps = 0;
  max_steps = 200;
  // while(!glm::all(glm::isnan(grid.get_norm_pos(extension)))&&std::abs(grid.eval_pos(extension)-0.1)>=0.01f&&num_steps<=max_steps){
  while (!glm::all(glm::isnan(grid.get_norm_pos(extension))) &&
         std::abs(grid.eval_pos(extension) - 0.01) >= 0.01f &&
         num_steps <= max_steps) {
    if (grid.eval_pos(extension) >= 0.1) {
      extension_local += direction;
    } else {
      extension_local -= direction;
    }
    extension = frame_to * glm::vec4(extension_local, 1.f);
    num_steps++;
  }
  ////// Texture Rotation
  // extension = find_extension_canoniso(from, frame_from, frame_to);
  num_steps = 0;
  max_steps = 10;
  glm::mat3 r =
      glm::rotate(glm::mat4(1.f), glm::pi<float>() / 40.f, glm::vec3(0, 1, 0));
  while (texture_grid.eval_pos(extension) > 0.f && num_steps <= max_steps) {
    extension_local = r * extension_local;
    extension = frame_to * glm::vec4(extension_local, 1.f);
    num_steps++;
  }
  direction = extension_local;
  if (glm::length2(direction) < 0.000001f) {
    glm::vec2 rand_dir = random_vec2();
    direction.x = rand_dir.x;
    direction.z = rand_dir.y;
  }
  direction = 0.0001f * glm::normalize(direction);
  num_steps = 0;
  max_steps = 200;
  while (!glm::all(glm::isnan(grid.get_norm_pos(extension))) &&
         std::abs(grid.eval_pos(extension) - 0.01) >= 0.01f &&
         num_steps <= max_steps) {
    if (grid.eval_pos(extension) >= 0.1) {
      extension_local += direction;
    } else {
      extension_local -= direction;
    }
    extension = frame_to * glm::vec4(extension_local, 1.f);
    num_steps++;
  }
  /////////////////////////////////////
  extension = from + segment_length * glm::normalize(extension - from);
  return extension;
}

std::optional<glm::vec3>
Strands::find_extension_canonptfeval(glm::vec3 from, glm::mat4 frame_from,
                                     glm::mat4 frame_to) {
  glm::vec3 target_point = frame_position(frame_to);
  glm::vec3 canonical_direction =
      glm::normalize(target_point - frame_position(frame_from));

  glm::mat4 inv_from = frame_inverse(frame_from);
  glm::mat4 inv_to = frame_inverse(frame_to);
  glm::vec3 local_pos = inv_from * glm::vec4(from, 1.f);
  local_pos.y = 0.f;

  // Init Eval Bounds
  float max_val_diff = 0.f;
  float min_val_diff = FLT_MAX;
  float max_local_dist2 = 0.f;
  float min_local_dist2 = FLT_MAX;
  float max_frame_dist2 = 0.f;
  float min_frame_dist2 = FLT_MAX;
  // Generate trials
  struct Trial {
    glm::vec3 global;
    glm::vec3 local;
    float val;
  };
  std::vector<Trial> trials;
  for (int i = 0; i < num_trials; i++) {
    glm::vec3 trial_head =
        from + segment_length *
                   random_vector(canonical_direction, glm::radians(max_angle));
    float val = grid.eval_pos(trial_head);
    if (val <= reject_iso) {
      glm::vec3 trial_local = inv_to * glm::vec4(trial_head, 1.f);
      glm::vec3 trial_local_projected =
          glm::vec3(trial_local.x, 0.f, trial_local.z);
      float distance = glm::distance(trial_head, target_point);
      float angle = glm::angle(trial_head - from, canonical_direction);
      trials.push_back({trial_head, trial_local, val});
      max_val_diff = std::max(max_val_diff, std::abs(val - target_iso));
      min_val_diff = std::min(min_val_diff, std::abs(val - target_iso));
      max_local_dist2 = std::max(
          max_local_dist2, glm::distance2(trial_local_projected, local_pos));
      min_local_dist2 = std::min(
          min_local_dist2, glm::distance2(trial_local_projected, local_pos));
      max_frame_dist2 = std::max(max_frame_dist2, glm::length2(trial_local));
      min_frame_dist2 = std::min(min_frame_dist2, glm::length2(trial_local));
    }
  }
  // If no valid trials add strand up to this moment
  if (trials.empty())
    return {};
  //  Evaluate trials
  Trial best_trial = trials[0];
  float best_fitness = 0.f;
  for (auto trial : trials) {
    glm::vec3 local_projected = glm::vec3(trial.local.x, 0.f, trial.local.z);
    float iso_metric =
        max_val_diff != min_val_diff
            ? iso_eval * (max_val_diff - (std::abs(trial.val - target_iso)) /
                                             (max_val_diff - min_val_diff))
            : 0.f;
    float local_metric =
        local_eval *
        ((max_local_dist2 - glm::distance2(local_projected, local_pos)) /
         (max_local_dist2 - min_local_dist2));
    float frame_metric =
        frame_eval * ((max_frame_dist2 - glm::length2(trial.local)) /
                      (max_frame_dist2 - min_frame_dist2));
    float fitness = iso_metric + local_metric + frame_metric;
    if (fitness >= best_fitness) {
      best_trial = trial;
      best_fitness = fitness;
    }
  }
  return best_trial.global;
}
std::optional<glm::vec3>
Strands::find_extension_ptfcanoneval(glm::vec3 from, glm::mat4 frame_from,
                                     glm::mat4 frame_to) {
  glm::vec3 target_point = frame_position(frame_to);
  glm::vec3 canonical_direction =
      glm::normalize(target_point - frame_position(frame_from));

  glm::mat4 inv_from = frame_inverse(frame_from);
  glm::mat4 inv_to = frame_inverse(frame_to);
  glm::vec3 local_pos = inv_from * glm::vec4(from, 1.f);
  local_pos.y = 0.f;

  // Init Eval Bounds
  float max_val_diff = 0.f;
  float min_val_diff = FLT_MAX;
  float max_trial_distance = 0.f;
  float min_trial_distance = FLT_MAX;
  float max_trial_angle = 0.f;
  // Generate trials
  struct Trial {
    glm::vec3 global;
    glm::vec3 local;
    float val;
    float distance;
    float angle;
  };
  std::vector<Trial> trials;
  for (int i = 0; i < num_trials; i++) {
    glm::vec2 sample = local_spread * random_vec2();
    glm::vec3 local_sample = local_pos;
    local_sample.x += sample.x;
    local_sample.z += sample.y;
    glm::vec3 global_sample = frame_to * glm::vec4(local_sample, 1.f);
    float val = grid.eval_pos(global_sample);
    if (val <= reject_iso) {
      float distance = glm::distance(global_sample, target_point);
      float angle = glm::angle(global_sample - from, canonical_direction);
      trials.push_back({global_sample, local_sample, val, distance, angle});
      // Update Evaluation Bounds
      max_val_diff = std::max(max_val_diff, std::abs(val - target_iso));
      min_val_diff = std::min(min_val_diff, std::abs(val - target_iso));
      max_trial_distance = fmax(max_trial_distance, distance);
      min_trial_distance = fmin(min_trial_distance, distance);
      max_trial_angle = fmax(max_trial_angle, angle);
    } else {
    }
  }
  // If no valid trials add strand up to this moment
  if (trials.empty())
    return {};
  //  Evaluate trials
  Trial best_trial = trials[0];
  float best_fitness = 0.f;
  for (auto trial : trials) {
    /*
    float iso_metric = max_val_diff != min_val_diff ?
        iso_eval*(max_val_diff-(std::abs(trial.val-target_iso))/(max_val_diff-min_val_diff))
        : 0.f;
        */
    float iso_metric = 0.f;
    float distance_metric =
        max_trial_distance != min_trial_distance
            ? frame_eval * (1 - (trial.distance - min_trial_distance) /
                                    (max_trial_distance - min_trial_distance))
            : 0.f;
    float direction_metric = local_eval * (1 - (trial.angle / max_trial_angle));
    float fitness = distance_metric + direction_metric + iso_metric;
    if (fitness >= best_fitness) {
      best_trial = trial;
      best_fitness = fitness;
    }
  }
  glm::vec3 extension =
      from + segment_length * glm::normalize(best_trial.global - from);
  return extension;
}

std::optional<glm::vec3> Strands::find_extension_texture(glm::vec3 from,
                                                         glm::mat4 frame_from,
                                                         glm::mat4 frame_to) {
  // New texture stepping tests
  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  glm::mat4 inv_from = frame_inverse(frame_from);
  glm::mat4 inv_to = frame_inverse(frame_to);
  // Calculate Position in local frame
  glm::vec3 local_pos = inv_from * glm::vec4(from, 1.f);
  local_pos.y = 0.f;

  // Init Eval Bounds
  float max_val_diff = 0.f;
  float min_val_diff = FLT_MAX;
  float max_local_dist2 = 0.f;
  float min_local_dist2 = FLT_MAX;
  float max_frame_dist2 = 0.f;
  float min_frame_dist2 = FLT_MAX;

  // Generate samples
  struct Trial {
    glm::vec3 local;
    glm::vec3 global;
    float val;
  };
  std::vector<Trial> trials;
  for (int i = 0; i < num_trials; i++) {
    glm::vec2 sample = local_spread * random_vec2();
    glm::vec3 local_sample = local_pos;
    local_sample.x += sample.x;
    local_sample.z += sample.y;
    glm::vec3 global_sample = frame_to * glm::vec4(local_sample, 1.f);
    float val = grid.eval_pos(global_sample);
    float tval = texture_grid.eval_pos(global_sample);
    if (val < 0.05f && tval < 0.05f) {
      trials.push_back({local_sample, global_sample, val});
      // Update Evaluation Bounds
      max_val_diff = std::max(max_val_diff, std::abs(val - target_iso));
      min_val_diff = std::min(min_val_diff, std::abs(val - target_iso));
      max_local_dist2 =
          std::max(max_local_dist2, glm::distance2(local_sample, local_pos));
      min_local_dist2 =
          std::min(min_local_dist2, glm::distance2(local_sample, local_pos));
      max_frame_dist2 = std::max(max_frame_dist2, glm::length2(local_sample));
      min_frame_dist2 = std::min(min_frame_dist2, glm::length2(local_sample));
    }
  }
  if (trials.empty())
    return {};

  Trial best_trial = trials[0];
  float best_fitness = -1.f;
  for (auto trial : trials) {
    float iso_metric =
        max_val_diff != min_val_diff
            ? iso_eval * (max_val_diff - (std::abs(trial.val - target_iso)) /
                                             (max_val_diff - min_val_diff))
            : 0.f;
    float local_metric =
        local_eval *
        ((max_local_dist2 - glm::distance2(trial.local, local_pos)) /
         (max_local_dist2 - min_local_dist2));
    float frame_metric =
        frame_eval * ((max_frame_dist2 - glm::length2(trial.local)) /
                      (max_frame_dist2 - min_frame_dist2));
    float fitness = iso_metric + local_metric + frame_metric;
    if (best_fitness <= fitness) {
      best_fitness = fitness;
      best_trial = trial;
    }
  }

  // return best sample
  glm::vec3 extension =
      from + segment_length * glm::normalize(best_trial.global - from);
  return extension;
  // return best_trial.global;
  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // return find_extension(from,frame_from,frame_to);
}

glm::vec3 Strands::move_extension(glm::vec3 head, glm::vec3 close, float iso) {
  // step towards closest until field is gets to reject value
  float a = 0.f, b = 1.f;
  glm::vec3 new_head = head;
  float val = grid.eval_pos(head);
  if (val > iso)
    return head;
  for (int i = 0; i < 16 && std::abs(val - iso) > 0.1; ++i) {
    float p = (b + a) / 2.f;
    new_head = (1.f - p) * close + p * head;
    val = grid.eval_pos(new_head);
    if (val > iso)
      a = p;
    else
      b = p;
  }
  return new_head;
}

Strands::TargetResult Strands::find_closest(glm::vec3 pos,
                                            const std::vector<glm::mat4> &path,
                                            int start_index, int end_index) {
  //  FIXME: CHECK THESE ASSERTIONS
  assert(start_index >= 0 && start_index < path.size());
  assert(end_index >= start_index && start_index < path.size());
  size_t closest_index = start_index;
  float lowest_dist2 = glm::distance2(pos, frame_position(path[closest_index]));

  for (int i = start_index; i <= end_index; i++) {
    float dist2 = glm::distance2(pos, frame_position(path[i]));
    if (dist2 < lowest_dist2) {
      lowest_dist2 = dist2;
      closest_index = i;
    }
  }
  return {closest_index, path[closest_index], lowest_dist2};
}

std::pair<size_t,size_t> Strands::match_root_all(glm::vec3 position) {
  position.y=0;
  float closest_d2=FLT_MAX;
  std::vector<std::pair<size_t,size_t>> matches={{0,0}};
  for (size_t i=0; i<root_frames.size(); ++i){
    std::vector<glm::mat4> &path=root_frames[i];
    for (size_t j=0; j<path.size(); ++j){
      glm::vec3 root_p = frame_position(path[j]); root_p.y=0;
      float d2 = glm::distance2(position,frame_position(path[j]));
      if (d2<closest_d2){
        closest_d2=d2;
        matches.clear();
        matches.push_back({i,j});
      }else if (d2==closest_d2){
        matches.push_back({i,j});
      }
    }
  }
  auto r = matches[(size_t)std::rand() % matches.size()];
  return r;
}
size_t Strands::match_root(glm::vec3 position, glm::mat4 frame) {
  if (root_pool.empty()) {
    if (select_pool == All) {
      for (size_t i = 0; i < root_frames.size(); ++i) {
          root_pool.push_back(i);
      }
    } else {
      root_pool.resize(root_frames.size());
      std::iota(root_pool.begin(), root_pool.end(), 0);
    }
  }
  size_t match_index = 0;
  if (select_method == AtRandom) {
    match_index = rng() % root_pool.size();
  } else if (select_method == WithAngle) {
    // CHECK  THIS
    std::vector<size_t> possible_matches = {};
    // Change this
    // glm::vec3 angle_vec =
    // glm::vec3(frame_inverse(frame)*glm::vec4(position,1.f));
    glm::vec3 angle_vec = position - tree.get_root_pos();
    // glm::vec3 angle_vec = position-frame_position(frame);
    angle_vec.y = 0.f;
    angle_vec = glm::normalize(angle_vec);
    double largest_cos = -1.f;
    assert("Root pool empty" && root_pool.size() != 0);
    for (size_t j = 0; j < root_pool.size(); j++) {
      double cos = glm::dot(angle_vec, root_vecs[root_pool[j]]);
      // double cos = glm::angle(angle_vec, root_vecs[root_pool[j]]);
      if (cos == largest_cos) {
        possible_matches.push_back(j);
      } else if (cos > largest_cos) {
        largest_cos = cos;
        possible_matches.clear();
        possible_matches.push_back(j);
      }
    }
    // assert(!possible_matches.empty());
    if (!possible_matches.empty()) {
      int i = std::rand() % possible_matches.size();
      // std::cout<<' '<<i<<' '<<possible_matches.size()<<std::endl;
      match_index = possible_matches[i];
    } else { // Shouldn't happen but idk
      std::cout << "No matches for: " << position << std::endl;
      // assert("No root matches" && !possible_matches.empty());
      match_index = (int)std::rand() % root_pool.size();
    }
    //
  }
  size_t match = root_pool[match_index];
  if (select_pool == NotSelected || select_pool == AtLeastOnce) {
    root_pool.erase(root_pool.begin() + match_index);
    if (root_pool.empty() && select_pool == AtLeastOnce) {
      select_pool = All;
    }
  }
  return match;
}
// Non-member helper functions
glm::vec3 random_vector(glm::vec3 axis, float angle) {
  const glm::vec3 x_axis(1, 0, 0);
  glm::quat rotation = glm::angleAxis(glm::angle(axis, x_axis),
                                      glm::normalize(glm::cross(x_axis, axis)));
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
  glm::vec3 v = rotation * glm::vec3(x, y, z);
  // Can be nan if axis is -x_axis
  if (glm::any(glm::isnan(v))){
    v = glm::vec3(x*axis.x, y, z);
  }
  assert(!glm::any(glm::isnan(v)));
  return v;
  // CODE CITED
}
glm::vec2 random_vec2() {
  std::uniform_real_distribution<float> r_rand(0.f, 1.f);
  std::uniform_real_distribution<float> theta_rand(0.f, 2 * glm::pi<float>());
  float r = std::sqrt(r_rand(rng));
  float theta = theta_rand(rng);
  float x = r * cos(theta);
  float y = r * sin(theta);
  return glm::vec2(x, y);
}

float lookahead_frame(float dist) {
  float u = dist / 0.05f;
  return (u + 1) * 2 * dist;
}
