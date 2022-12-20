#pragma once

// Light blue
//#define SKY_COLOR glm::vec4(.75f,1.f,1.f, 1.f)
// Darker blue
//#define SKY_COLOR glm::vec4(.2f,.2f,.5f, 1.f)
// Foggy
#define SKY_COLOR glm::vec4(0.529,0.808,0.922,1.0)


// Tree specific constants
#define FOCUS glm::vec3(0,1.5,0)
#define DISTANCE 5.f
#define DIMENSIONS glm::ivec3(500,500,500)
#define SCALE .006f
#define CENTER glm::vec3(0.2,1.5,0)
// STRAND CONSTANTS
#define SEGMENT_LENGTH .05f
#define NUM_TRIALS 50
#define MAX_ANGLE 90.f // TODO WHY DOES INCREASING THIS MAKE IT TIGHTER???????
#define ALPHA 0.5f

/*
// For poplar
#define FOCUS glm::vec3(0,1.5,0)
#define DISTANCE 5.f
#define DIMENSIONS glm::ivec3(500,500,500)
#define SCALE .006f
#define CENTER glm::vec3(0.2,1.5,0)
// STRAND CONSTANTS
#define SEGMENT_LENGTH .05f
#define NUM_TRIALS 100
#define MAX_ANGLE 200.f // TODO WHY DOES INCREASING THIS MAKE IT TIGHTER???????
#define ALPHA 0.5f

// For malus baccata columnaris
#define FOCUS glm::vec3(0,1.3,0)
#define DISTANCE 5.f
#define DIMENSIONS glm::ivec3(375,1000,375)
#define SCALE .0022f
#define CENTER glm::vec3(0,1.05,0)
// STRAND CONSTANTS
#define SEGMENT_LENGTH .01f
#define NUM_TRIALS 100
#define MAX_ANGLE 200.f // TODO WHY DOES INCREASING THIS MAKE IT TIGHTER???????
#define ALPHA 0.5f

// For nice tree
#define FOCUS glm::vec3(0,0.3,0)
#define DISTANCE 2.f
#define DIMENSIONS glm::ivec3(650,390,650)
#define SCALE .0014f
#define CENTER glm::vec3(0,0.25,0)
// STRAND CONSTANTS
#define SEGMENT_LENGTH .003f
#define NUM_TRIALS 50
#define MAX_ANGLE 200.f // TODO WHY DOES INCREASING THIS MAKE IT TIGHTER???????
#define ALPHA 0.1f

// For bush ra17
#define FOCUS glm::vec3(0,0.5,0)
#define DISTANCE 2.f
#define DIMENSIONS glm::ivec3(500,480,480)
#define SCALE .0022f
#define CENTER glm::vec3(0.08,0.5,0)
// STRAND CONSTANTS
#define SEGMENT_LENGTH .001f
#define NUM_TRIALS 30
#define MAX_ANGLE 200.f // TODO WHY DOES INCREASING THIS MAKE IT TIGHTER???????
#define ALPHA 0.5f

// For tall ellipsoid tree
#define FOCUS glm::vec3(0,0.8,0)
#define DISTANCE 3.f
#define DIMENSIONS glm::ivec3(400,500,400)
#define SCALE .0035f
#define CENTER glm::vec3(0,0.84,0)

// For oak 4
#define FOCUS glm::vec3(0,0.2,0)
#define DISTANCE 2.7f
#define DIMENSIONS glm::ivec3(580,340,580)
#define SCALE .0026f
#define CENTER glm::vec3(0.03,0.4,0)

// For gnarled
#define FOCUS glm::vec3(0,0.3,0)
#define DISTANCE 2.f
#define DIMENSIONS glm::ivec3(650,390,650)
#define SCALE .002f
#define CENTER glm::vec3(0,0.32,0)
*/
