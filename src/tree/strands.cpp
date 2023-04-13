#include "strands.h"
#include "const.h"
#include "tree/implicit.h"
#include "util/geometry.h"
#include <glm/gtx/io.hpp>

std::default_random_engine
    rng(std::chrono::system_clock::now().time_since_epoch().count());

glm::vec3 random_vector(glm::vec3 axis, float angle);

Strands::Strands(const Skeleton &tree, Grid &grid, Implicit &evalfunc) : 
  grid(grid),evalfunc(evalfunc)
{
  for (size_t i = 0; i < tree.leafs_size(); i++) {
    paths.push_back(tree.get_strand(i));
  }
}
void Strands::add_strands(unsigned int amount) {
  std::cout << "Generating Strands...";
  std::cout.flush();
  std::vector<size_t> indices;
  for (size_t i = 0; i < paths.size(); i++) {
    indices.push_back(i);
  }
  std::shuffle(std::begin(indices), std::end(indices), rng);
  for (size_t i = 0; i < amount; i++) {
    add_strand(indices[i % (indices.size())]);
  }
  std::cout << " Done" << std::endl;
}

// FOR TESTING
void print_actual_closest(glm::vec3 pos, const std::vector<glm::vec3> &path) {
  float closest_dist = FLT_MAX;
  int closest_ind = 0;
  for (int i = 0; i < path.size(); i++) {
    float curr_dist = glm::distance(pos, path[i]);
    if (closest_dist > curr_dist) {
      closest_dist = curr_dist;
    }
    closest_ind = i;
  }
  std::cout << "ACTUAL: " << closest_ind << " " << closest_dist << std::endl;
}

// THE ALGORITHM THAT IMPLEMENTS STRAND VOXEL AUTOMATA
void Strands::add_strand(size_t path_index) {
  // Set up strand
  if (path_index >= paths.size())
    return;
  const std::vector<glm::vec3> &path = paths[path_index];
  size_t closest_index = 0;
  glm::vec3 last_closest = path[closest_index];
  std::vector<glm::vec3> strand{path[closest_index]};

  // Loop until closest node is last node
  while (closest_index != path.size() - 1) {
    // Start of this segment is head of last
    glm::vec3 start(strand[strand.size() - 1]);

    // Find Target point
    // TODO: Extract to function
    size_t target_index = closest_index;
    glm::vec3 target_point = path[target_index];
    float travelled = 0.f;
    float distance_to_travel =
        SEGMENT_LENGTH + glm::distance(target_point, start);
    bool found_target = false;
    while (!found_target) {
      if (target_index == path.size() - 1) {
        // Bound target point to tree root
        target_point = path[target_index];
        found_target = true;
      } else if (travelled > distance_to_travel) {
        // Backtrack and travel exactly distance needed
        target_index--;
        glm::vec3 last_step = path[target_index + 1] - path[target_index];
        travelled -= glm::length(last_step);
        float left_to_travel = distance_to_travel - travelled;
        target_point =
            path[target_index] + glm::normalize(last_step) * left_to_travel;
        found_target = true;
      } else {
        // Travel down the path
        travelled += glm::distance(path[target_index], path[target_index + 1]);
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
    std::vector<Trial>
        trials; // TODO already know the size of the vector so allocate it
    float max_distance = 0.f;
    float min_distance = FLT_MAX;
    float max_angle = 0.f;
    for (int i = 0; i < NUM_TRIALS; i++) {
      // glm::vec3 trial_head =
      // start+distance_to_travel*random_vector(canonical_direction,
      // glm::radians(MAX_ANGLE));
      glm::vec3 trial_head =
          start + SEGMENT_LENGTH * random_vector(canonical_direction,
                                                 glm::radians(MAX_ANGLE));
      float val = grid.get_in_pos(trial_head);
      //if (!grid.line_occluded(start, trial_head)) { // Original validation condition
      if (val<=REJECT_VAL) {
        float distance = glm::distance(trial_head, target_point);
        float angle = glm::angle(trial_head - start, canonical_direction);
        trials.push_back({trial_head, distance, angle});
        max_distance = fmax(max_distance, distance);
        min_distance = fmin(min_distance, distance);
        max_angle = fmax(max_angle, angle);
      } // else std::cout<<"REJECTED"<<std::endl;
    }
    // std::cout<<trials.size()<<std::endl;

    // If no valid trials add strand up to this moment
    if (trials.empty()) //TODO: could change this to back up
      break;
    // if(trials.empty()) return;

    //  Evaluate trials
    int best_trial = 0;
    float best_fitness = 0.f;
    for (int i = 1; i < trials.size(); i++) {
      float distance_metric = 1 - (trials[i].distance - min_distance) /
                                      (max_distance - min_distance);
      float direction_metric = 1 - (trials[i].angle / max_angle);
      float fitness = ALPHA * distance_metric + (1 - ALPHA) * direction_metric;
      if (fitness >= best_fitness) {
        best_trial = i;
        best_fitness = fitness;
      }
    }
    // Add best trial calculate new closest index
    // DEBUG TO SEE TRIALS
    // strand.push_back(trials[best_trial].head);
    // strand.push_back(target_point);
    /*
    for ( auto trial : trials ){
        strand.push_back(trial.head);
    }
    */

    // closest_index=target_index;
    strand.push_back(trials[best_trial].head);
    closest_index = closest_node_on_path(strand.back(), path, target_index, 5).first;
    /*
    std::cout<<std::endl;
    std::cout<<"FOUND: "<<closest_index<<" "<<glm::distance(strand.back(),
    path[closest_index])<<std::endl; //TODO DEBUG PRINT
    print_actual_closest(strand.back(),path);
    */
  }
  // Occupy strand path
  if (strand.size()<=2) return;
  strands.push_back(strand);
  //grid.occupy_path(strand, 1);
  //grid.fill_path(strand, evalfunc);
  grid.fill_path_2(strand, evalfunc);
  /*
  for (auto point : strand){
    grid.fill_point(point,evalfunc);
  }
  */
}

Mesh<Vertex> Strands::get_mesh() const {
  std::vector<Vertex> vertices;
  std::vector<GLuint> indices;
  for (auto path : strands) {
    glm::vec3 color = random_brown();
    size_t start_index = vertices.size();
    for (auto position : path) {
      vertices.push_back(Vertex{position, color});
    }
    size_t end_index = vertices.size();
    for (int i = start_index; i < end_index - 1; i++) {
      indices.push_back(i);
      indices.push_back(i + 1);
    }
  }
  /*
  for (size_t i = 0; i < vertices.size(); i++){
      indices.push_back(i);
  }
  */
  return Mesh(vertices, indices);
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
  return rotation * glm::vec3(x, y, z);
  // CODE CITED
}
