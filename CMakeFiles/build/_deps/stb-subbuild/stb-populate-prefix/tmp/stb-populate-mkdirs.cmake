# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "E:/github/NightRain2/build/_deps/stb-src")
  file(MAKE_DIRECTORY "E:/github/NightRain2/build/_deps/stb-src")
endif()
file(MAKE_DIRECTORY
  "E:/github/NightRain2/build/_deps/stb-build"
  "E:/github/NightRain2/build/_deps/stb-subbuild/stb-populate-prefix"
  "E:/github/NightRain2/build/_deps/stb-subbuild/stb-populate-prefix/tmp"
  "E:/github/NightRain2/build/_deps/stb-subbuild/stb-populate-prefix/src/stb-populate-stamp"
  "E:/github/NightRain2/build/_deps/stb-subbuild/stb-populate-prefix/src"
  "E:/github/NightRain2/build/_deps/stb-subbuild/stb-populate-prefix/src/stb-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "E:/github/NightRain2/build/_deps/stb-subbuild/stb-populate-prefix/src/stb-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "E:/github/NightRain2/build/_deps/stb-subbuild/stb-populate-prefix/src/stb-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
