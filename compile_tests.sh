#!/bin/bash

echo "C++ tests compilation is disabled. The project has been converted to C only."
echo "If you need testing, please implement C-only tests."
exit 0

# NOTE: Le code ci-dessous est conservé au cas où vous voudriez implémenter des tests en C pur à l'avenir

# # Script to compile and run tests
# set -e
# 
# # Directory where the script is located
# SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
# 
# # Create build directory if it doesn't exist
# mkdir -p "$SCRIPT_DIR/build_tests"
# cd "$SCRIPT_DIR/build_tests"
# 
# # Configure with CMake
# cmake -G "Unix Makefiles" \
#       -DCMAKE_BUILD_TYPE=Debug \
#       "$SCRIPT_DIR/test"
# 
# # Build tests
# cmake --build . -- -j$(nproc)
# 
# # Run tests if specified
# if [ "$1" == "run" ]; then
#     # Run the tests
#     ctest --output-on-failure
# fi
