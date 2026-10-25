cmake_minimum_required(VERSION 3.16)
project(WorldMechanicsModules VERSION 0.1.0 LANGUAGES CXX)

option(WM_BUILD_TESTS "Build portable mechanics tests" ON)
add_library(world_mechanics
  WorldClassification/src/classification_map.cpp
  WorldClassification/src/landscape_builder.cpp
  MeshCollision/src/cpu_surface_mesher.cpp
  TerrainVoxel/src/terrain_voxel.cpp
  Excavation/src/excavation.cpp
  Shoring/src/shoring.cpp
  Persistence/src/density_snapshot_codec.cpp
  StructuralIntegrity/src/structural_integrity.cpp
  Survival/src/survival.cpp
  WorldEnvironment/src/environment_simulation.cpp
  GameplayIntegration/src/gameplay_quality.cpp
  GameplayIntegration/src/local_stone_ring_recognition.cpp
  GameplayIntegration/src/stone_fireplace_lifecycle.cpp
  GameplayIntegration/src/trap_lifecycle.cpp
  PlacementSnap/src/world_snap_two_anchor_solver.cpp
)
target_compile_features(world_mechanics PUBLIC cxx_std_17)
target_include_directories(world_mechanics PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/CoreContracts/include
  ${CMAKE_CURRENT_SOURCE_DIR}/RuntimeProfiles/include
  ${CMAKE_CURRENT_SOURCE_DIR}/WorldScan/include
  ${CMAKE_CURRENT_SOURCE_DIR}/WorldClassification/include
  ${CMAKE_CURRENT_SOURCE_DIR}/TerrainVoxel/include
  ${CMAKE_CURRENT_SOURCE_DIR}/MeshCollision/include
  ${CMAKE_CURRENT_SOURCE_DIR}/PlacementSnap/include
  ${CMAKE_CURRENT_SOURCE_DIR}/Excavation/include
  ${CMAKE_CURRENT_SOURCE_DIR}/Shoring/include
  ${CMAKE_CURRENT_SOURCE_DIR}/StructuralIntegrity/include
  ${CMAKE_CURRENT_SOURCE_DIR}/Survival/include
  ${CMAKE_CURRENT_SOURCE_DIR}/WorldEnvironment/include
  ${CMAKE_CURRENT_SOURCE_DIR}/GameplayIntegration/include
  ${CMAKE_CURRENT_SOURCE_DIR}/Persistence/include
)
if(MSVC)
  target_compile_options(world_mechanics PRIVATE /W4 /permissive-)
else()
  target_compile_options(world_mechanics PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(WM_BUILD_TESTS)
  enable_testing()
  add_executable(world_mechanics_tests tests/module_tests.cpp)
  target_link_libraries(world_mechanics_tests PRIVATE world_mechanics)
  add_test(NAME world_mechanics_tests COMMAND world_mechanics_tests)
endif()
