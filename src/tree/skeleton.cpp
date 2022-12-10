#include "tree/skeleton.h"
#include <iostream>

// FOR DEBUG
#define PRINT_POS(pos)  \
    std::cout<<pos.x<<" "<<pos.y<<" "<<pos.z<<std::endl \

#define PRINT(string) std::cout<<string<<std::endl

// TODO: I feel like this is messy
Skeleton::Skeleton(const char* filename){
    // Initialize file stream, and string token
    std::ifstream in(filename);
    std::string token;

    #define GET_NEXT(token) std::getline(in,token, ' ')
        
    /*
    #define PARSE_POINT(token, pos)     \
        for (int i = 0; i<3; i++){      \
            GET_NEXT(token);            \
            pos[i] = std::stof(token);  \
        }                               \
        GET_NEXT(token);                \
        if(token.find(")")==std::string::npos)    \
            throw std::invalid_argument( "Incorrect file format: position parentheses not closed" )    \
    */
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
    root = std::make_shared<Node>(Node{});
    // Get root start
    GET_NEXT(token);
    if (token.find("(")==std::string::npos)
        throw std::invalid_argument( "Incorrect file format: Opening parentheses for position not found" );
    PARSE_POINT(token, root->position);
    // Get root end
    /*
    GET_NEXT(token);
    if (token.find("(")==std::string::npos)
        throw std::invalid_argument( "Incorrect file format: Opening parentheses for position not found" );
    PARSE_POINT(token, root->end);
    */

    std::stack<std::shared_ptr<Node>> last_split;
    last_split.push(root);
    std::shared_ptr<Node> last_node=root;

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
            glm::vec3 position;
            PARSE_POINT(token,position);
            std::shared_ptr<Node>next=std::make_shared<Node>(Node{position,last_node,{}});
            // Define node's relationship
            next->parent = last_node;
            last_node->children.push_back(next);
            // Update last_node
            last_node=next;
        }
    }
    leafs.push_back(last_node);
}

Mesh Skeleton::get_mesh(){
    // Initialize mesh verts and indices
    glm::vec3 col(0.6,0,0.8);
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
    struct Info{
        std::shared_ptr<Node> node;
        GLuint index;   // Index of the node's end point
        GLuint parent_index=0;   // Index of the node's end point
        unsigned int children_explored = 0;
    };
    Info last_node{root, 0};
    std::stack<Info> last_split;
    //last_split.push(last_node);
    bool done = false;
    int debug_counter=0;
    while (!done){
        unsigned int num_children = last_node.node->children.size();
        unsigned int explored = last_node.children_explored;
        // Add node to vertices (first time node is reached)
        if (explored==0){
            vertices.push_back(Vertex{last_node.node->position,col});
            indices.push_back(last_node.parent_index);
            indices.push_back(last_node.index);
        }

        // See how to continue
        if(num_children==0){
            // Reached leaf go back to last split
            last_split.top().children_explored++;
            if ( last_split.empty() ) done = true;
            else last_node=last_split.top();
        } else if(num_children==1){
            // Straight path
            last_node = {last_node.node->children[0],(GLuint)vertices.size(), last_node.index};
        }else{
            if (explored<num_children){
                // Children left to explore
                if (last_split.top().index!=last_node.index){
                    last_split.push(last_node);
                }
                last_node = {last_node.node->children[last_node.children_explored],(GLuint)vertices.size(), last_node.index};
            }else{
                // Children all explored
                last_split.pop();
                if(!last_split.empty()){
                    last_split.top().children_explored++;
                    last_node = last_split.top();
                }else done = true;
            }
        }
        //if (debug_counter++>10) break;
    }
    return Mesh(vertices, indices); 
}
