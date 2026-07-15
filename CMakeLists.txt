cmake_minimum_required(VERSION 3.28)

project(obs-beauty-filter VERSION 0.1.0 LANGUAGES C)

include(CTest)

set(OBS_SOURCE_DIR "" CACHE PATH "Path to an OBS Studio source tree matching the target OBS version")
set(OBS_LIBRARY "" CACHE FILEPATH "Path to the libobs library or framework binary")

if(NOT EXISTS "${OBS_SOURCE_DIR}/libobs/obs-module.h")
  message(FATAL_ERROR "Set OBS_SOURCE_DIR to an OBS Studio source tree containing libobs/obs-module.h")
endif()

if(NOT OBS_LIBRARY)
  if(APPLE AND EXISTS "/Applications/OBS.app/Contents/Frameworks/libobs.framework/libobs")
    set(OBS_LIBRARY "/Applications/OBS.app/Contents/Frameworks/libobs.framework/libobs")
  else()
    message(FATAL_ERROR "Set OBS_LIBRARY to the libobs binary for the target OBS installation")
  endif()
endif()

add_library(obs-beauty-filter MODULE src/obs-beauty-filter.c src/face_tracker.c)

target_include_directories(obs-beauty-filter PRIVATE "${OBS_SOURCE_DIR}/libobs")
target_link_libraries(obs-beauty-filter PRIVATE "${OBS_LIBRARY}")
target_compile_features(obs-beauty-filter PRIVATE c_std_11)

if(MSVC)
  target_compile_options(obs-beauty-filter PRIVATE /W4)
else()
  target_compile_options(obs-beauty-filter PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(BUILD_TESTING)
  add_executable(face-tracker-test tests/face_tracker_test.c src/face_tracker.c)
  target_include_directories(face-tracker-test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
  target_compile_features(face-tracker-test PRIVATE c_std_11)
  if(NOT MSVC)
    target_link_libraries(face-tracker-test PRIVATE m)
  endif()
  add_test(NAME face-tracker COMMAND face-tracker-test)
endif()

set_target_properties(obs-beauty-filter PROPERTIES PREFIX "" OUTPUT_NAME "obs-beauty-filter")

if(APPLE)
  set_target_properties(obs-beauty-filter PROPERTIES
    BUNDLE TRUE
    BUNDLE_EXTENSION plugin
    MACOSX_BUNDLE_BUNDLE_NAME "OBS Beauty Filter"
    MACOSX_BUNDLE_BUNDLE_IDENTIFIER "com.obs-beauty-filter.plugin"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    CMAKE_OSX_DEPLOYMENT_TARGET "12.0"
  )

  add_custom_command(TARGET obs-beauty-filter POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_BUNDLE_CONTENT_DIR:obs-beauty-filter>/Resources"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
      "${CMAKE_CURRENT_SOURCE_DIR}/data/beauty.effect"
      "$<TARGET_BUNDLE_CONTENT_DIR:obs-beauty-filter>/Resources/beauty.effect"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_BUNDLE_CONTENT_DIR:obs-beauty-filter>/Resources/locale"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
      "${CMAKE_CURRENT_SOURCE_DIR}/data/locale/zh-CN.ini"
      "$<TARGET_BUNDLE_CONTENT_DIR:obs-beauty-filter>/Resources/locale/zh-CN.ini"
  )

  install(TARGETS obs-beauty-filter
    BUNDLE DESTINATION "."
    LIBRARY DESTINATION "."
  )
  # `install_name_tool` may adjust RPATH while installing a bundle, which
  # invalidates the build-time ad-hoc signature. Re-sign the staged bundle;
  # release packaging replaces `-` with the configured Developer ID identity.
  install(CODE [[
    execute_process(
      COMMAND codesign --force --sign - --timestamp=none
        "${CMAKE_INSTALL_PREFIX}/obs-beauty-filter.plugin"
      COMMAND_ERROR_IS_FATAL ANY
    )
  ]])
else()
  install(TARGETS obs-beauty-filter
    RUNTIME DESTINATION "obs-beauty-filter/bin/64bit"
    LIBRARY DESTINATION "obs-beauty-filter/bin/64bit"
  )
  install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/data/beauty.effect"
    DESTINATION "obs-beauty-filter/data/obs-plugins/obs-beauty-filter")
  install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/data/locale/zh-CN.ini"
    DESTINATION "obs-beauty-filter/data/obs-plugins/obs-beauty-filter/locale")
endif()
