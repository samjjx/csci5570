include_directories(${PROJECT_SOURCE_DIR} ${HUSKY_EXTERNAL_INCLUDE})

#add_library(csci5570 STATIC
#  $<TARGET_OBJECTS:server-objs>
#  $<TARGET_OBJECTS:comm-objs>
#  $<TARGET_OBJECTS:base-objs>
#  $<TARGET_OBJECTS:io-objs>
#  $<TARGET_OBJECTS:driver-objs>
#  )

#file(GLOB_RECURSE UNITTEST_FILES ${PROJECT_SOURCE_DIR}/*_test.cpp)

include_directories(${GTEST_INCLUDE})
add_executable(SvmApp ${PROJECT_SOURCE_DIR}/app/svm.cpp)
add_dependencies(SvmApp gtest)
target_link_libraries(SvmApp csci5570)
target_link_libraries(SvmApp ${HUSKY_EXTERNAL_LIB})
target_link_libraries(SvmApp ${GTEST_LIBRARIES})
target_link_libraries(SvmApp ${GMOCK_LIBRARIES})
set_property(TARGET SvmApp PROPERTY CXX_STANDARD 11)
add_dependencies(SvmApp ${external_project_dependencies})

