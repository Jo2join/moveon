cmake_minimum_required(VERSION 3.10)


project(websocket_service)



set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)


set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -Wall -g -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE"${CMAKE_CXX_FLAGS_RELEASE} -O3 -Wall")

# find_package(yaml-cpp REQUIRED)

include_directories(
    # ${YAML_INCLUDE_DIRS}
    /usr/include/jsoncpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../utils
    ${CMAKE_CURRENT_SOURCE_DIR}/server
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
    ${CMAKE_CURRENT_SOURCE_DIR}/service
    ${CMAKE_CURRENT_SOURCE_DIR}/message
)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} SRC)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/message SRC)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/service SRC)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/server SRC)
# aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/../common/src COMMON_SRC)

add_executable(${PROJECT_NAME} 
               ${SRC}
               ${COMMON_SRC}
               
)


target_link_libraries(${PROJECT_NAME} 
    # ${YAML_CPP_LIBRARIES}
    jsoncpp
    pthread
    dl
    boost_system
)

# install(FILES cfg/config.yaml  DESTINATION ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}/cfg)
# install(FILES cfg/test.json DESTINATION ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}/cfg)

# install(TARGETS ${PROJECT_NAME}  DESTINATION ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}/bin)




