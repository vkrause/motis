file(GLOB_RECURSE motis-photon-files src/*.cc)
add_library(motis-photon STATIC ${motis-photon-files})
target_link_libraries(motis-photon
  motis-module
  motis-core
  motis-libjson
)
target_include_directories(motis-photon PUBLIC include)
target_compile_features(motis-photon PUBLIC cxx_std_17)
target_compile_options(motis-photon PRIVATE ${MOTIS_CXX_FLAGS})
