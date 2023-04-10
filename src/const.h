#pragma once

// Light blue
//#define SKY_COLOR glm::vec4(.75f,1.f,1.f, 1.f)
// Darker blue
//#define SKY_COLOR glm::vec4(.2f,.2f,.5f, 1.f)
// Foggy
#define SKY_COLOR glm::vec4(0.529,0.808,0.922,1.0)

/*
// Test Camera
#define FOCUS glm::vec3(0,1.f,0)
#define DISTANCE 5.f
*/

// Tree specific constants
#define FOCUS glm::vec3(0, 1.5, 0)
#define DISTANCE 5.f
#define GRIDSCALEFACTOR 0.4f
// STRAND CONSTANTS
#define SEGMENT_LENGTH .03f
#define NUM_TRIALS 50
#define MAX_ANGLE 90.f
#define ALPHA 0.5f
// IMPLICIT CONSTANTS
#define RADIUS 0.005f
#define BLOBINESS -1.f
#define CUTOFF_VAL 0.05f
#define REJECT_VAL 2.f
#define SURFACE_VAL 1.f
#define OFFSET 0.2f

/*
// For poplar
#define FOCUS glm::vec3(0, 1.5, 0)
#define DISTANCE 5.f
#define GRIDSCALEFACTOR 0.4f
// STRAND CONSTANTS
#define SEGMENT_LENGTH .03f
#define NUM_TRIALS 50
#define MAX_ANGLE 90.f
#define ALPHA 0.5f
// IMPLICIT CONSTANTS
#define RADIUS 0.005f
#define BLOBINESS -1.f
#define CUTOFF_VAL 0.05f
#define REJECT_VAL 2.f
#define SURFACE_VAL 1.f
#define OFFSET 0.2f

// For malus baccata columnaris
#define FOCUS glm::vec3(0, 1.5, 0)
#define DISTANCE 5.f
#define GRIDSCALEFACTOR 0.2f
// STRAND CONSTANTS
#define SEGMENT_LENGTH .03f
#define NUM_TRIALS 50
#define MAX_ANGLE 90.f
#define ALPHA 0.5f
// IMPLICIT CONSTANTS
#define RADIUS 0.003f
#define BLOBINESS -.4f
#define CUTOFF_VAL 0.05f
#define REJECT_VAL 5.f
#define SURFACE_VAL 1.f
#define OFFSET 0.22f

// For nice tree
#define FOCUS glm::vec3(0,0.3,0)
#define DISTANCE 2.f
#define GRIDSCALEFACTOR 0.2f
// STRAND CONSTANTS
#define SEGMENT_LENGTH .003f
#define NUM_TRIALS 10
#define MAX_ANGLE 200.f
#define ALPHA 0.1f
// IMPLICIT CONSTANTS
#define RADIUS 0.002f
#define BLOBINESS -1.f
#define CUTOFF_VAL 0.1f
#define REJECT_VAL 0.6f
#define SURFACE_VAL 1.f

// For bush ra17
#define FOCUS glm::vec3(0,0.5,0)
#define DISTANCE 2.f
#define GRIDSCALEFACTOR 0.6f
// STRAND CONSTANTS
#define SEGMENT_LENGTH .001f
#define NUM_TRIALS 10
#define MAX_ANGLE 200.f
#define ALPHA 0.5f
// IMPLICIT CONSTANTS
#define RADIUS 0.003f
#define BLOBINESS -1.f
#define CUTOFF_VAL 0.01f
#define REJECT_VAL 0.6f
#define SURFACE_VAL 1.f

// For tall ellipsoid tree
#define FOCUS glm::vec3(0,0.8,0)
#define DISTANCE 3.f
#define GRIDSCALEFACTOR 0.6f
// STRAND CONSTANTS
#define SEGMENT_LENGTH .03f
#define NUM_TRIALS 50
#define MAX_ANGLE 90.f
#define ALPHA 0.5f
// IMPLICIT CONSTANTS
#define RADIUS 0.003f
#define BLOBINESS -0.5f
#define CUTOFF_VAL 0.5f
#define REJECT_VAL 3.f
#define SURFACE_VAL 1.f

// For oak 4
#define FOCUS glm::vec3(0,0.2,0)
#define DISTANCE 2.7f

// For gnarled
#define FOCUS glm::vec3(0,0.3,0)
#define DISTANCE 2.f


// For Irregular Crown
#define FOCUS glm::vec3(0,0.3,0)
#define DISTANCE 2.f
//#define GRIDSCALEFACTOR 1.f
#define GRIDSCALEFACTOR 0.5f
// STRAND CONSTANTS
#define SEGMENT_LENGTH .03f
#define NUM_TRIALS 50
#define MAX_ANGLE 90.f
#define ALPHA 0.5f
// IMPLICIT CONSTANTS
#define RADIUS 0.005f
#define BLOBINESS -5.f
#define CUTOFF_VAL 0.5f
#define REJECT_VAL 10.f
#define SURFACE_VAL 1.f
*/
