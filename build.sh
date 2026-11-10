#!/usr/bin/env bash
set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$PROJECT_ROOT"

echo "==> [1/3] Подготовка каталога сборки..."
mkdir -p build
cd build

echo "==> [2/3] Сборка проекта и тестов..."
cmake .. > /dev/null
cmake --build . -j$(nproc) > /dev/null

echo "==> [3/3] Выполнение тестов..."

# Запуск Catch2 с перенаправлением полного отчета в файл
LOG_FILE="test_failures.log"
set +e
./tests/unit_tests -r console -o "$LOG_FILE"
TEST_STATUS=$?
set -e

echo ""
echo "==============================================================================="
# Вывод только блока итогов (последние строки отчета)
grep -E "(test cases:|assertions:)" "$LOG_FILE" || tail -n 4 "$LOG_FILE"
echo "==============================================================================="

if [ $TEST_STATUS -ne 0 ]; then
    echo "❌ Найдены ошибки! Полный подробный отчет записан в:"
    echo "   $PROJECT_ROOT/build/$LOG_FILE"
    exit $TEST_STATUS
else
    echo "✅ Все тесты пройдены без ошибок!"
    rm -f "$LOG_FILE"
fi