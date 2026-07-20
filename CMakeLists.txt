cmake_minimum_required(VERSION 3.28)

project(obs-beauty-filter VERSION 0.1.0 LANGUAGES C CXX)

include(CTest)

if(APPLE)
  if(NOT CMAKE_OSX_ARCHITECTURES STREQUAL "arm64")
    message(FATAL_ERROR "obs-beauty-filter supports Apple Silicon arm64 only; configure with CMAKE_OSX_ARCHITECTURES=arm64")
  endif()
elseif(WIN32)
  if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "obs-beauty-filter supports Windows x64 only")
  endif()
else()
  message(FATAL_ERROR "obs-beauty-filter supports Windows x64 and Apple Silicon macOS only")
endif()

set(OBS_SOURCE_DIR "" CACHE PATH "Path to an OBS Studio source tree matching the target OBS version")
set(OBS_LIBRARY "" CACHE FILEPATH "Path to the libobs library or framework binary")
option(OBS_BEAUTY_ENABLE_MEDIAPIPE "Build with the MediaPipe Face Landmarker runtime" OFF)
set(MEDIAPIPE_ROOT "" CACHE PATH "Path to the pinned MediaPipe source tree")
set(MEDIAPIPE_FACE_LANDMARKER_LIBRARY "" CACHE FILEPATH
  "Path to the MediaPipe Face Landmarker shared library")
set(MEDIAPIPE_FACE_LANDMARKER_RUNTIME "" CACHE FILEPATH
  "Path to the MediaPipe Face Landmarker runtime DLL (required on Windows)")
set(MEDIAPIPE_RUNTIME_LIBRARIES "" CACHE STRING
  "Semicolon-separated runtime libraries required by MediaPipe (for example OpenCV)")

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

set(OBS_BEAUTY_SOURCES
  src/obs-beauty-filter.c
  src/beauty_preset.c
  src/beauty_sampling.c
  src/beauty_settings_migration.c
  src/face_tracker.c
  src/face_landmarks.c
)

if(OBS_BEAUTY_ENABLE_MEDIAPIPE)
  if(NOT EXISTS "${MEDIAPIPE_ROOT}/mediapipe/tasks/c/vision/face_landmarker/face_landmarker.h")
    message(FATAL_ERROR "Set MEDIAPIPE_ROOT to the pinned MediaPipe source tree")
  endif()
  if(NOT EXISTS "${MEDIAPIPE_FACE_LANDMARKER_LIBRARY}")
    message(FATAL_ERROR "Set MEDIAPIPE_FACE_LANDMARKER_LIBRARY to the built Face Landmarker shared library")
  endif()
  if(WIN32 AND NOT EXISTS "${MEDIAPIPE_FACE_LANDMARKER_RUNTIME}")
    message(FATAL_ERROR "Set MEDIAPIPE_FACE_LANDMARKER_RUNTIME to the Face Landmarker runtime DLL")
  endif()
  if(NOT MEDIAPIPE_RUNTIME_LIBRARIES)
    message(FATAL_ERROR "Set MEDIAPIPE_RUNTIME_LIBRARIES to every shared runtime library required by MediaPipe")
  endif()
  foreach(runtime_library IN LISTS MEDIAPIPE_RUNTIME_LIBRARIES)
    if(NOT EXISTS "${runtime_library}")
      message(FATAL_ERROR "MediaPipe runtime library does not exist: ${runtime_library}")
    endif()
  endforeach()
  find_package(Threads REQUIRED)
  list(APPEND OBS_BEAUTY_SOURCES
    src/mediapipe_face_landmarker.cpp
    src/face_inference_worker.cpp
    src/face_frame_bridge.c
  )
endif()

add_library(obs-beauty-filter MODULE ${OBS_BEAUTY_SOURCES})

target_include_directories(obs-beauty-filter PRIVATE "${OBS_SOURCE_DIR}/libobs")
target_link_libraries(obs-beauty-filter PRIVATE "${OBS_LIBRARY}")
target_compile_features(obs-beauty-filter PRIVATE c_std_11)

if(OBS_BEAUTY_ENABLE_MEDIAPIPE)
  target_compile_definitions(obs-beauty-filter PRIVATE OBS_BEAUTY_ENABLE_MEDIAPIPE=1)
  target_include_directories(obs-beauty-filter PRIVATE "${MEDIAPIPE_ROOT}")
  target_link_libraries(obs-beauty-filter PRIVATE
    "${MEDIAPIPE_FACE_LANDMARKER_LIBRARY}"
    Threads::Threads
  )
  target_compile_features(obs-beauty-filter PRIVATE cxx_std_20)
  if(APPLE)
    set_target_properties(obs-beauty-filter PROPERTIES
      BUILD_WITH_INSTALL_RPATH TRUE
      INSTALL_RPATH "@loader_path/../Frameworks;/Applications/OBS.app/Contents/Frameworks"
    )
  endif()
endif()

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

  add_executable(face-landmarks-test tests/face_landmarks_test.c src/face_landmarks.c)
  target_include_directories(face-landmarks-test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
  target_compile_features(face-landmarks-test PRIVATE c_std_11)
  if(NOT MSVC)
    target_link_libraries(face-landmarks-test PRIVATE m)
  endif()
  add_test(NAME face-landmarks COMMAND face-landmarks-test)

  add_executable(beauty-preset-test tests/beauty_preset_test.c src/beauty_preset.c)
  target_include_directories(beauty-preset-test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
  target_compile_features(beauty-preset-test PRIVATE c_std_11)
  add_test(NAME beauty-preset COMMAND beauty-preset-test)
  # The Xcode generator can omit test-only executables from ALL_BUILD.
  add_dependencies(face-tracker-test beauty-preset-test)

  add_executable(beauty-sampling-test tests/beauty_sampling_test.c src/beauty_sampling.c)
  target_include_directories(beauty-sampling-test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
  target_compile_features(beauty-sampling-test PRIVATE c_std_11)
  add_test(NAME beauty-sampling COMMAND beauty-sampling-test)
  add_dependencies(face-tracker-test beauty-sampling-test)

  add_executable(beauty-settings-migration-test
    tests/beauty_settings_migration_test.c
    src/beauty_settings_migration.c
  )
  target_include_directories(beauty-settings-migration-test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
  target_compile_features(beauty-settings-migration-test PRIVATE c_std_11)
  add_test(NAME beauty-settings-migration COMMAND beauty-settings-migration-test)
  add_dependencies(face-tracker-test beauty-settings-migration-test)

  add_executable(obs-module-smoke-test tests/obs_module_smoke_test.c)
  target_include_directories(obs-module-smoke-test PRIVATE "${OBS_SOURCE_DIR}/libobs")
  target_link_libraries(obs-module-smoke-test PRIVATE "${OBS_LIBRARY}")
  target_compile_features(obs-module-smoke-test PRIVATE c_std_11)
  if(APPLE)
    add_test(NAME obs-module-smoke
      COMMAND obs-module-smoke-test
        "$<TARGET_BUNDLE_CONTENT_DIR:obs-beauty-filter>/MacOS/obs-beauty-filter"
        "$<TARGET_BUNDLE_CONTENT_DIR:obs-beauty-filter>/Resources")
  else()
    add_test(NAME obs-module-smoke
      COMMAND obs-module-smoke-test
        "$<TARGET_FILE:obs-beauty-filter>"
        "${CMAKE_CURRENT_SOURCE_DIR}/data")
  endif()
  # The test needs the post-build plugin binary and its resources/runtime.
  add_dependencies(obs-module-smoke-test obs-beauty-filter)
  add_dependencies(face-tracker-test obs-module-smoke-test)

  if(OBS_BEAUTY_ENABLE_MEDIAPIPE)
    add_executable(face-inference-test
      tests/mediapipe_face_inference_test.cpp
      src/mediapipe_face_landmarker.cpp
      src/face_landmarks.c
      src/face_tracker.c
    )
    target_include_directories(face-inference-test PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/src"
      "${MEDIAPIPE_ROOT}"
    )
    target_link_libraries(face-inference-test PRIVATE "${MEDIAPIPE_FACE_LANDMARKER_LIBRARY}")
    target_link_libraries(face-inference-test PRIVATE ${MEDIAPIPE_RUNTIME_LIBRARIES})
    target_compile_features(face-inference-test PRIVATE cxx_std_20)
    add_test(NAME face-inference
      COMMAND face-inference-test "${CMAKE_CURRENT_SOURCE_DIR}/models/face_landmarker.task")
    add_executable(face-inference-worker-test
      tests/face_inference_worker_test.cpp
      src/face_inference_worker.cpp
      src/mediapipe_face_landmarker.cpp
      src/face_landmarks.c
      src/face_tracker.c
    )
    target_include_directories(face-inference-worker-test PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/src"
      "${MEDIAPIPE_ROOT}"
    )
    target_link_libraries(face-inference-worker-test PRIVATE
      "${MEDIAPIPE_FACE_LANDMARKER_LIBRARY}"
      ${MEDIAPIPE_RUNTIME_LIBRARIES}
      Threads::Threads
    )
    target_compile_features(face-inference-worker-test PRIVATE cxx_std_20)
    add_test(NAME face-inference-worker
      COMMAND face-inference-worker-test "${CMAKE_CURRENT_SOURCE_DIR}/models/face_landmarker.task")
    if(APPLE)
      # The inference tests exercise the native MediaPipe runtime directly.
      # Make them load the exact Frameworks copy placed in the P1 bundle so
      # CTest remains reproducible when the original third-party build tree
      # is no longer present on the developer machine.
      set_tests_properties(face-inference face-inference-worker PROPERTIES
        ENVIRONMENT "DYLD_LIBRARY_PATH=$<TARGET_BUNDLE_CONTENT_DIR:obs-beauty-filter>/Frameworks"
      )
    endif()
    # The Xcode generator can omit a test-only executable from ALL_BUILD.
    # Keep the inference smoke test in the default P1 build through an
    # existing default target, so `ctest` never sees an absent executable.
    add_dependencies(face-tracker-test face-inference-test face-inference-worker-test)
  endif()
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

  if(OBS_BEAUTY_ENABLE_MEDIAPIPE)
    add_custom_command(TARGET obs-beauty-filter POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_BUNDLE_CONTENT_DIR:obs-beauty-filter>/Frameworks"
      COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${MEDIAPIPE_FACE_LANDMARKER_LIBRARY}"
        "$<TARGET_BUNDLE_CONTENT_DIR:obs-beauty-filter>/Frameworks/"
      COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/models/face_landmarker.task"
        "$<TARGET_BUNDLE_CONTENT_DIR:obs-beauty-filter>/Resources/face_landmarker.task"
    )
    foreach(runtime_library IN LISTS MEDIAPIPE_RUNTIME_LIBRARIES)
      add_custom_command(TARGET obs-beauty-filter POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
          "${runtime_library}"
          "$<TARGET_BUNDLE_CONTENT_DIR:obs-beauty-filter>/Frameworks/"
      )
    endforeach()
  endif()

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
  if(OBS_BEAUTY_ENABLE_MEDIAPIPE)
    # Keep DLL dependencies beside the build artifact so Windows CTest loads
    # the same native runtime that will later be installed into bin/64bit.
    add_custom_command(TARGET obs-beauty-filter POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${MEDIAPIPE_FACE_LANDMARKER_RUNTIME}"
        "$<TARGET_FILE_DIR:obs-beauty-filter>"
    )
    foreach(runtime_library IN LISTS MEDIAPIPE_RUNTIME_LIBRARIES)
      add_custom_command(TARGET obs-beauty-filter POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
          "${runtime_library}"
          "$<TARGET_FILE_DIR:obs-beauty-filter>"
      )
    endforeach()
  endif()
  install(TARGETS obs-beauty-filter
    RUNTIME DESTINATION "obs-beauty-filter/bin/64bit"
    LIBRARY DESTINATION "obs-beauty-filter/bin/64bit"
  )
  install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/data/beauty.effect"
    DESTINATION "obs-beauty-filter/data/obs-plugins/obs-beauty-filter")
  install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/data/locale/zh-CN.ini"
    DESTINATION "obs-beauty-filter/data/obs-plugins/obs-beauty-filter/locale")
  if(OBS_BEAUTY_ENABLE_MEDIAPIPE)
    install(FILES "${MEDIAPIPE_FACE_LANDMARKER_RUNTIME}"
      DESTINATION "obs-beauty-filter/bin/64bit")
    install(FILES ${MEDIAPIPE_RUNTIME_LIBRARIES}
      DESTINATION "obs-beauty-filter/bin/64bit")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/models/face_landmarker.task"
      DESTINATION "obs-beauty-filter/data/obs-plugins/obs-beauty-filter")
  endif()
endif()
