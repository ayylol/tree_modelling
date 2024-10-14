#pragma once

#include <vector>
#include <stack>
#include <array>
#include <memory>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cerrno>
#include <stdexcept>
#include <regex>

#include <glm/glm.hpp>
#include <nlohmann/json.hpp>
#include "rendering/mesh.h"
#include "util/color.h"

glm::vec3 random_color();
class Skeleton{
    public:
        Skeleton(nlohmann::json& options); 

        Mesh<VertFlat> get_mesh();

        size_t leafs_size() const;
        size_t roots_size() const;
        std::pair<glm::vec3,glm::vec3> get_bounds() const;
        glm::vec3 get_com() const;
        glm::vec3 get_root_pos() const;
        glm::mat4 get_root_frame() const;
        float get_average_length() const;

        enum path_type{
          LEAF,
          ROOT
        };
        std::vector<glm::mat4> get_strand(size_t index, path_type type=LEAF) const;

        struct Node{
            glm::mat4 frame;
            std::shared_ptr<Node> parent;
            std::vector<std::shared_ptr<Node>>children;
        }; 
        std::shared_ptr<Node> shoot_root;
        std::vector<std::shared_ptr<Node>> leafs;
        std::shared_ptr<Node> root_root;
        std::vector<std::shared_ptr<Node>> root_tips;

    private:
        struct ParseInfo{
            std::pair<glm::vec3,glm::vec3> extent;
            glm::vec3 center_of_mass;
            int num_nodes;
            float total_length;
        };
        enum Direction{
            FORWARDS,
            BACKWARDS
        };
        static ParseInfo parse(std::shared_ptr<Node>& root,  
            std::vector<std::shared_ptr<Node>>& leafs,
            std::string filename,
            glm::mat4 init_frame=glm::mat4(0.f),
            Direction dir=FORWARDS);
        void transform();
        void transform_dfs(Node& node, glm::mat4 t, glm::mat4 s, glm::mat4 r);
        void calculate_stats();
        void stats_dfs(Node& node, ParseInfo& stats);

        ParseInfo shoot_stats;
        ParseInfo root_stats;
        std::pair<glm::vec3,glm::vec3> bounds;
        glm::vec3 center_of_mass;
        float average_length;

        bool root_zup;
        bool shoot_zup;
};
