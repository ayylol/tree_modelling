#include "tree/skeleton.h"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtx/transform.hpp"
#include "glm/gtx/vector_angle.hpp"
#include <glm/gtx/io.hpp>
#include <iostream>
#include <utility>
#include <util/geometry.h>

using json = nlohmann::json;

size_t Skeleton::leafs_size() const {return leafs.size();}
size_t Skeleton::roots_size() const {return root_tips.size();}
std::pair<glm::vec3,glm::vec3> Skeleton::get_bounds() const {return bounds;}
glm::vec3 Skeleton::get_com() const {return center_of_mass;}
glm::vec3 Skeleton::get_root_pos() const {return frame_position(shoot_root->frame);}
float Skeleton::get_average_length() const {return average_length;}


Skeleton::Skeleton(json& options){
    auto shoot_stats = parse(shoot_root, leafs, options.at("tree_file"));
    auto root_stats = parse(root_root, root_tips, options.at("root_file"),shoot_root->frame,BACKWARDS);
    center_of_mass = shoot_stats.center_of_mass;
    average_length = (shoot_stats.total_length+root_stats.total_length)/(shoot_stats.num_nodes+root_stats.num_nodes);
    bounds.first.x =  fmin(shoot_stats.extent.first.x, root_stats.extent.first.x);
    bounds.first.y =  fmin(shoot_stats.extent.first.y, root_stats.extent.first.y);
    bounds.first.z =  fmin(shoot_stats.extent.first.z, root_stats.extent.first.z);
    bounds.second.x = fmax(shoot_stats.extent.second.x,root_stats.extent.second.x);
    bounds.second.y = fmax(shoot_stats.extent.second.y,root_stats.extent.second.y);
    bounds.second.z = fmax(shoot_stats.extent.second.z,root_stats.extent.second.z);

    std::cout<<"---- Skeleton Stats ----"<<std::endl;
    std::cout<<"Root Position: "<<frame_position(shoot_root->frame)<<std::endl;
    std::cout<<"Number of Leafs: "<< leafs_size()<<std::endl;
    std::cout<<"Number of Roots: "<< roots_size()<<std::endl;
    std::cout<<"Average Segment Length: "<< average_length<<std::endl;
    std::cout<<"Max Extent: "<<bounds.first<<" "<<bounds.second<<std::endl;
    std::cout<<"------------------------"<<std::endl;
}

Mesh<VertFlat> Skeleton::get_mesh(){
    // Initialize mesh verts and indices
    const float axis_scale=0.003f;
    const VertFlat axis[6] = {
        {glm::vec3(0,0,0),glm::vec3(1,0,0)},{axis_scale*glm::vec3(1,0,0),glm::vec3(1,0,0)},
        {glm::vec3(0,0,0),glm::vec3(0,1,0)},{axis_scale*glm::vec3(0,1,0),glm::vec3(0,1,0)},
        {glm::vec3(0,0,0),glm::vec3(0,0,1)},{axis_scale*glm::vec3(0,0,1),glm::vec3(0,0,1)}
    };
    std::vector<VertFlat> vertices;
    std::vector<GLuint> indices;
    struct Info{
        std::shared_ptr<Node> node;
        GLuint index;   // Index of the node's end point
        GLuint parent_index=0;   // Index of the node's end point
        unsigned int children_explored = 0;
    };
    Info last_node{shoot_root, 0};
    std::stack<Info> last_split;
    bool done = false;
    bool done_shoot = false;
    int debug_counter=0;
    while (!done){
        unsigned int num_children = last_node.node->children.size();
        unsigned int explored = last_node.children_explored;
        // Add node to vertices (first time node is reached)
        if (explored==0){
            if (!done_shoot){
                //std::cout<<frame_position(last_node.node->frame)<<std::endl;
            }
            for (auto vert : axis){
                VertFlat v=vert;
                v.position = glm::vec3(last_node.node->frame*glm::vec4(v.position,1.f));
                vertices.push_back(v);
                indices.push_back(vertices.size()-1);
            }
        }

        // See how to continue
        if(num_children==0){
            // Reached leaf go back to last split
            if ( last_split.empty() ) {
                done = true;
            }
            else {
                last_split.top().children_explored++;
                last_node=last_split.top();
            }
        } else if(num_children==1){
            // Straight path
            last_node = {last_node.node->children[0],(GLuint)vertices.size(), last_node.index};
        }else{
            if (explored<num_children){
                // Children left to explore
                if (last_split.empty() || last_split.top().index!=last_node.index){
                    last_split.push(last_node);
                }
                last_node = {last_node.node->children[last_node.children_explored],(GLuint)vertices.size(), last_node.index};
            }else{
                // Children all explored
                if(!last_split.empty()) last_split.pop();
                if(!last_split.empty()){
                    last_split.top().children_explored++;
                    last_node = last_split.top();
                }else done = true;
            }
        }
        if (done && !done_shoot){
          done_shoot = true;
          done = false;
          last_node = {root_root, 0};
        }
    }
    return Mesh(vertices, indices); 
}

std::vector<glm::mat4> Skeleton::get_strand(size_t index, path_type type) const
{
    auto &paths = type == LEAF ? leafs : root_tips;
    if ( index >= paths.size() || index < 0) {
        std::cout<<"Not a valid strand"<<std::endl;
        return std::vector<glm::mat4>();
    }
    std::vector<glm::mat4> strand;
    std::shared_ptr<Node> current = paths[index];
    while ( current != nullptr){
        strand.push_back(current->frame);
        current = current->parent;
    }
    return strand;
}


Skeleton::ParseInfo Skeleton::parse(std::shared_ptr<Node>& root,
                                  std::vector<std::shared_ptr<Node>>& leafs,
                                  std::string filename, 
                                  glm::mat4 init_frame,
                                  Direction dir) {
    // Initialize file stream, and string token
    std::ifstream in(filename);
    std::string token;

    #define GET_NEXT(token) std::getline(in,token, ' ')
    #define PARSE_POINT(token, pos)     \
        GET_NEXT(token);                \
        pos.x = std::stof(token);       \
        GET_NEXT(token);                \
        pos.z = std::stof(token);       \
        GET_NEXT(token);                \
        pos.y = std::stof(token);       \
        GET_NEXT(token);                \
        if(token.find(")")==std::string::npos)    \
            throw std::invalid_argument( "Incorrect file format: position parentheses not closed" )    \
    
    // Initialize root node
    root = std::make_shared<Skeleton::Node>(Skeleton::Node{});
    // Get root start
    GET_NEXT(token);
    if (token.find("(")==std::string::npos)
        throw std::invalid_argument( "Incorrect file format: Opening parentheses for position not found" );
    glm::vec3 position;
    PARSE_POINT(token, position);
    root->frame = glm::translate(position);

    std::stack<std::shared_ptr<Skeleton::Node>> last_split;
    last_split.push(root);
    std::shared_ptr<Skeleton::Node> last_node=root;

    // Setting up stats
    ParseInfo stats = {
            .extent = std::make_pair(frame_position(root->frame), frame_position(root->frame)),
            .center_of_mass = frame_position(root->frame),
            .num_nodes = 1,
            .total_length = 0
    };
    bool after_root = true;
    for (std::string token; GET_NEXT(token);) 
    {
        // Branch starting
        if(token.find("[")!=std::string::npos){
            last_split.push(last_node);
        }
        // Branch ending
        if(token.find("]")!=std::string::npos){
            // Save onto fringe list
            leafs.push_back(last_node); 
            // Go back to the branch split point
            last_node = last_split.top();
            last_split.pop();
        }

        // Point being specified
        if(token.find("(")!=std::string::npos){
            // Define position of the node  
            PARSE_POINT(token,position);
            std::shared_ptr<Node>next=
                std::make_shared<Node>(Node{
                        .frame = glm::translate(position),
                        .parent = last_node,
                        .children = {}
                        });

            // Define node's relationship
            next->parent = last_node;
            last_node->children.push_back(next);
            // Update Frame
            if (after_root) {
                after_root = false;
                if (init_frame != glm::mat4(0.f)){
                    last_node->frame = init_frame; 
                } else { // Calculate Frame
                    // Get prev and this tangent
                    glm::vec3 tangent = glm::normalize(dir == FORWARDS ? 
                        position-frame_position(next->parent->frame) : 
                        frame_position(next->parent->frame)-position);
                    // Transform N and B to new frame
                    glm::vec3 normal = glm::vec3(1,0,0);
                    glm::vec3 binormal = glm::cross(tangent,normal);
                    normal = glm::cross(tangent,binormal);
                    // Update new frame
                    last_node->frame = last_node->frame*glm::mat4(
                            glm::vec4(normal,0),
                            glm::vec4(tangent,0),
                            glm::vec4(binormal,0),
                            glm::vec4(0,0,0,1));
                }
            }
            // Get prev and this tangent
            glm::vec3 tangent = glm::normalize(dir == FORWARDS ? 
                position-frame_position(next->parent->frame) : 
                frame_position(next->parent->frame)-position);
            glm::vec3 last_tangent = next->parent->frame*glm::vec4(0,1,0,0);
            // Get rotation to new frame
            glm::vec3 rot_axis = glm::cross(last_tangent,tangent);
            float angle = glm::angle(last_tangent,tangent);
            glm::mat4 rotation = glm::rotate(glm::mat4(1.f),angle,rot_axis);
            if (rot_axis==glm::vec3(0,0,0)){
               rotation=glm::mat4(1.f); 
            }
            // Transform N and B to new frame
            glm::vec3 normal = rotation*next->parent->frame*glm::vec4(1,0,0,0);
            glm::vec3 binormal = rotation*next->parent->frame*glm::vec4(0,0,1,0);
            // Update new frame
            next->frame = next->frame*glm::mat4(
                    glm::vec4(normal,0),
                    glm::vec4(tangent,0),
                    glm::vec4(binormal,0),
                    glm::vec4(0,0,0,1));

            // Update last node
            last_node=next;

            // Updates for stats
            // Bounds
            stats.extent.first.x = fmin(stats.extent.first.x,position.x);
            stats.extent.first.y = fmin(stats.extent.first.y,position.y);
            stats.extent.first.z = fmin(stats.extent.first.z,position.z);
            stats.extent.second.x = fmax(stats.extent.second.x,position.x);
            stats.extent.second.y = fmax(stats.extent.second.y,position.y);
            stats.extent.second.z = fmax(stats.extent.second.z,position.z);
            // COM
            stats.center_of_mass += position;
            // length
            //std::cout<<glm::distance(position,frame_position(next->parent->frame))<<std::endl;
            stats.total_length += glm::distance(position, frame_position(next->parent->frame));
            stats.num_nodes++;
        }
    }
    //std::cout<<std::endl;
    leafs.push_back(last_node);
    in.close();
    stats.center_of_mass *= (1.f/stats.num_nodes);
    return stats;
}
