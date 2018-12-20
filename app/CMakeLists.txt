include_directories(${PROJECT_SOURCE_DIR} ${HUSKY_EXTERNAL_INCLUDE})

#add_library(csci5570 STATIC
#  $<TARGET_OBJECTS:server-objs>
#  $<TARGET_OBJECTS:comm-objs>
#  $<TARGET_OBJECTS:base-objs>
#  $<TARGET_OBJECTS:io-objs>
#  $<TARGET_OBJECTS:driver-objs>
#  )

#file(GLOB_RECURSE UNITTEST_FILES ${PROJECT_SOURCE_DIR}/*_test.cpp)
find_package(Eigen3 REQUIRED)

include_directories(${GTEST_INCLUDE} ${EIGEN3_INCLUDE_DIRS})
add_executable(Logistic ${PROJECT_SOURCE_DIR}/app/run_logistic.cpp)
#add_dependencies(Logistic gtest)
target_link_libraries(Logistic csci5570)
target_link_libraries(Logistic ${HUSKY_EXTERNAL_LIB})
#target_link_libraries(Logistic ${GTEST_LIBRARIES})
#target_link_libraries(Logistic ${GMOCK_LIBRARIES})
set_property(TARGET Logistic PROPERTY CXX_STANDARD 11)
add_dependencies(Logistic ${external_project_dependencies})

add_executable(SVM ${PROJECT_SOURCE_DIR}/app/SVM.cpp)

target_link_libraries(SVM csci5570)
target_link_libraries(SVM ${HUSKY_EXTERNAL_LIB})
set_property(TARGET SVM PROPERTY CXX_STANDARD 11)
add_dependencies(SVM ${external_project_dependencies})
