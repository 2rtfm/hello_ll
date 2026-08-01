set(USER_Include_Dirs
  ${CMAKE_CURRENT_SOURCE_DIR}/../../User/Inc
)

file(GLOB_RECURSE USER_Application_Src CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/../../User/Src/*.c"
)

target_include_directories(stm32cubemx INTERFACE ${USER_Include_Dirs})
target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${USER_Application_Src})
