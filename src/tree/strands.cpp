#include "strands.h"
#include <glm/gtx/io.hpp>

std::default_random_engine rng(std::chrono::system_clock::now().time_since_epoch().count());

Strands::Strands(const Skeleton& tree, Grid& grid):
    grid(grid)
{
   for ( size_t i = 0; i<tree.leafs_size(); i++){
        paths.push_back(tree.get_strand(i));
   }
}
void Strands::add_strands(unsigned int amount){
    std::vector<size_t> indices;
    for (size_t i = 0; i < paths.size(); i++){
        indices.push_back(i);
    }
    std::shuffle(std::begin(indices),std::end(indices),rng);
    for (size_t i = 0; i < amount; i++){
        add_strand(indices[i%(indices.size())]);
    }
}

glm::vec3 random_vector(glm::vec3 axis, float angle){
    const glm::vec3 x_axis(1,0,0);
    glm::quat rotation = glm::angleAxis(glm::angle(axis,x_axis),glm::normalize(glm::cross(x_axis,axis)));

    // CODE CITED
    // from https://community.khronos.org/t/random-vectors/41467/3 imported_jwatte
    std::uniform_real_distribution<float> x_rand(glm::cos(angle),1);
    std::uniform_real_distribution<float> a_rand(-glm::pi<float>(), glm::pi<float>());
    float x=x_rand(rng);
    float a=a_rand(rng);
    float r=sqrtf(1-x*x);
    float y=glm::sin(a)*r;
    float z=glm::cos(a)*r;
    return rotation*glm::vec3(x,y,z);
    // CODE CITED
}

#define SEGMENT_LENGTH .05f
#define NUM_TRIALS 100
#define MAX_ANGLE 200.f // TODO WHY DOES INCREASING THIS MAKE IT TIGHTER???????
#define ALPHA 0.5f
void Strands::add_strand(size_t path_index){
    if ( path_index >= paths.size()) return;
    const std::vector<glm::vec3>& path = paths[path_index];
    size_t closest_index = 0;
    glm::vec3 last_closest = path[closest_index];
    std::vector<glm::vec3> strand{path[closest_index]};
    // Loop until closest node is last node
    while ( closest_index != path.size()-1 ){
        std::cout<<closest_index<<std::endl;

        // Start of this segment is head of last
        glm::vec3 start(strand[strand.size()-1]);

        // Travel a distance down path
        size_t target_index = closest_index;
        glm::vec3 target_point = path[target_index];
        float travelled = 0.f;
        float distance_to_travel = SEGMENT_LENGTH+glm::distance(target_point, start);
        bool found_target= false;
        while ( !found_target ){
            if (target_index == path.size()-1){
                // Bound target point to tree root
                target_point = path[target_index];
                found_target = true;
            }else if ( travelled > distance_to_travel ){
                // Backtrack and travel exactly distance needed
                target_index--;
                glm::vec3 last_step = path[target_index+1]-path[target_index];
                travelled-=glm::length(last_step);
                float left_to_travel = distance_to_travel - travelled;
                target_point = path[target_index]+glm::normalize(last_step)*left_to_travel;
                found_target = true;
            }else{
                // Travel down the path
                travelled+=glm::distance(path[target_index],path[target_index+1]);
                target_index++;
            }
        }
        // use target point to calculate canonical direction
        glm::vec3 canonical_direction = glm::normalize(target_point-last_closest);
        //std::cout<<canonical_direction<<std::endl;

        // Generate trials
        struct Trial{
            glm::vec3 head;
            float distance;
            float angle;
        };
        std::vector<Trial> trials; // TODO already know the size of the vector so allocate it
        float max_distance=0.f;
        float min_distance=FLT_MAX;
        float max_angle=0.f;
        for (int i = 0; i<NUM_TRIALS;i++){
            glm::vec3 trial_head = start+distance_to_travel*random_vector(canonical_direction, glm::radians(MAX_ANGLE));
            if (!grid.line_occluded(start, trial_head)){
                float distance= glm::distance(trial_head,target_point);
                float angle =  glm::angle(trial_head-start,canonical_direction);
                trials.push_back({
                        trial_head,
                        distance,
                        angle
                        });
                max_distance = fmax(max_distance,distance);
                min_distance = fmin(min_distance,distance);
                max_angle = fmax(max_angle, angle);
            }else std::cout<<"REJECTED"<<std::endl;
        }
        if(trials.empty()) break;
        // evaluate trial
        int best_trial = 0;
        float best_fitness = 0.f;
        for (int i = 1; i<trials.size(); i++){
            float distance_metric = 1 - (trials[i].distance-min_distance)/(max_distance-min_distance);
            float direction_metric = 1 - (trials[i].angle/max_angle);
            float fitness = ALPHA*distance_metric+(1-ALPHA)*direction_metric;
            if (fitness >= best_fitness){
                best_trial=i;
                best_fitness=fitness;
            }
        }
        // Add best trial calculate new closest index
        strand.push_back(trials[best_trial].head);
        closest_index=target_index;
        
    }
    // Occupy strand path
    strands.push_back(strand);
    grid.occupy_path(strand, 1);
}

Mesh Strands::get_mesh() const{
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
    for ( auto path : strands ) {
        glm::vec3 color = random_color();
        for ( auto position : path ) {
            vertices.push_back(Vertex{position, color});
        }
    }
    for (size_t i = 0; i < vertices.size(); i++){
        indices.push_back(i);
    }
    return Mesh(vertices, indices);
}
