cmake_minimum_required(VERSION 3.16)

#Required for embedding glade files
set(PYTHON_EXECUTABLE python3)

project(LaserWorks)

#Creating main target
include_directories(include)
file(GLOB sources "src/*.cpp" "src/*.h")
add_executable(LaserWorks ${sources})
target_link_libraries(LaserWorks -lpthread)

#GTKMM linking and including
find_package(PkgConfig)
pkg_check_modules(GTKMM gtkmm-3.0)
link_directories(${GTKMM_LIBRARY_DIRS})
include_directories(${GTKMM_INCLUDE_DIRS})
target_link_libraries(LaserWorks ${GTKMM_LIBRARIES})

#Embedding glade files in glade.h header
if(PYTHON_EXECUTABLE)
    add_custom_target(
        EmbedResources ALL
        COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/embed-resources.py ${PROJECT_SOURCE_DIR}/resources.xml ${PROJECT_SOURCE_DIR}/src/resources.h -n res
        COMMENT "Embedding resources."
    )
    add_dependencies(LaserWorks EmbedResources)
else()
    message(WARNING "Python3 not found. Resource files won't be embedded.")
endif()