# Install script for directory: /afs/andrew.cmu.edu/usr4/ymao1/Desktop/cmu-15462-assignment5-YuMao-RuiZhu/src

# Set the install prefix
IF(NOT DEFINED CMAKE_INSTALL_PREFIX)
  SET(CMAKE_INSTALL_PREFIX "/usr/local")
ENDIF(NOT DEFINED CMAKE_INSTALL_PREFIX)
STRING(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
IF(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  IF(BUILD_TYPE)
    STRING(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  ELSE(BUILD_TYPE)
    SET(CMAKE_INSTALL_CONFIG_NAME "")
  ENDIF(BUILD_TYPE)
  MESSAGE(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
ENDIF(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)

# Set the component getting installed.
IF(NOT CMAKE_INSTALL_COMPONENT)
  IF(COMPONENT)
    MESSAGE(STATUS "Install component: \"${COMPONENT}\"")
    SET(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  ELSE(COMPONENT)
    SET(CMAKE_INSTALL_COMPONENT)
  ENDIF(COMPONENT)
ENDIF(NOT CMAKE_INSTALL_COMPONENT)

# Install shared libraries without execute permission?
IF(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  SET(CMAKE_INSTALL_SO_NO_EXE "0")
ENDIF(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)

IF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  IF(EXISTS "$ENV{DESTDIR}/afs/andrew.cmu.edu/usr4/ymao1/Desktop/cmu-15462-assignment5-YuMao-RuiZhu/pathtracer" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/afs/andrew.cmu.edu/usr4/ymao1/Desktop/cmu-15462-assignment5-YuMao-RuiZhu/pathtracer")
    FILE(RPATH_CHECK
         FILE "$ENV{DESTDIR}/afs/andrew.cmu.edu/usr4/ymao1/Desktop/cmu-15462-assignment5-YuMao-RuiZhu/pathtracer"
         RPATH "")
  ENDIF()
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/afs/andrew.cmu.edu/usr4/ymao1/Desktop/cmu-15462-assignment5-YuMao-RuiZhu/pathtracer")
  IF (CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  ENDIF (CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
  IF (CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  ENDIF (CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
FILE(INSTALL DESTINATION "/afs/andrew.cmu.edu/usr4/ymao1/Desktop/cmu-15462-assignment5-YuMao-RuiZhu" TYPE EXECUTABLE FILES "/afs/andrew.cmu.edu/usr4/ymao1/Desktop/cmu-15462-assignment5-YuMao-RuiZhu/build/pathtracer")
  IF(EXISTS "$ENV{DESTDIR}/afs/andrew.cmu.edu/usr4/ymao1/Desktop/cmu-15462-assignment5-YuMao-RuiZhu/pathtracer" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/afs/andrew.cmu.edu/usr4/ymao1/Desktop/cmu-15462-assignment5-YuMao-RuiZhu/pathtracer")
    IF(CMAKE_INSTALL_DO_STRIP)
      EXECUTE_PROCESS(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}/afs/andrew.cmu.edu/usr4/ymao1/Desktop/cmu-15462-assignment5-YuMao-RuiZhu/pathtracer")
    ENDIF(CMAKE_INSTALL_DO_STRIP)
  ENDIF()
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

