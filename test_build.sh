#!/usr/bin/env bash
set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$PROJECT_ROOT"

echo "==> [1/3] Staging the build dir..."
mkdir -p build
cd build

echo "==> [2/3] Building the project and tests..."
cmake "$PROJECT_ROOT" -DBUILD_TESTING=ON > /dev/null
cmake --build . -j$(nproc) > /dev/null

echo "==> [3/3] Testing..."

LOG_FILE="test_failures.log"
set +e
./tests/unit_tests -r console -o "$LOG_FILE"
TEST_STATUS=$?
set -e

echo ""
echo "==============================================================================="
grep -E "(test cases:|assertions:)" "$LOG_FILE" || tail -n 4 "$LOG_FILE"
echo "==============================================================================="

if [ $TEST_STATUS -ne 0 ]; then
    echo "❌ Error, log in:"
    echo "   $PROJECT_ROOT/build/$LOG_FILE"
    exit $TEST_STATUS
else
    rm -f "$LOG_FILE"
fi
