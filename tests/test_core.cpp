#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators_all.hpp>
#include <filesystem>
#include <fstream>
#include <random>
#include <thread>
#include <vector>

#include "exception.hpp"
#include "logger.hpp"
#include "source_manager.hpp"

TEST_CASE("Core: Exceptions randomized message verification",
          "[core][exception]") {
    auto random_row =
        GENERATE(take(25, Catch::Generators::random(1u, 100000u)));
    auto random_col = GENERATE(take(25, Catch::Generators::random(1u, 500u)));

    SECTION("syntax_error coordinates formatting") {
        std::string      err_msg = "Random syntax failure";
        vm::syntax_error err(err_msg.c_str(), random_row, random_col);
        std::string      what = err.what();

        REQUIRE(what.find(err_msg) != std::string::npos);
        REQUIRE(what.find("row: " + std::to_string(random_row)) !=
                std::string::npos);
        REQUIRE(what.find("col: " + std::to_string(random_col)) !=
                std::string::npos);
    }
}

TEST_CASE("Core: SourceManager boundaries and randomized access",
          "[core][source_manager]") {
    SECTION("Empty vector triggers invalid_argument") {
        std::vector<Token> empty_tokens;
        REQUIRE_THROWS_AS(SourceManager(empty_tokens), std::invalid_argument);
    }

    SECTION("Fuzzing token storage with random vector sizes") {
        std::mt19937_64                       rng(1337);
        std::uniform_int_distribution<size_t> size_dist(10, 200);
        size_t                                count = size_dist(rng);

        std::vector<Token> tokens;
        tokens.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            tokens.emplace_back(TokenType::NUMBER,
                                Position{static_cast<uint32_t>(i), 1,
                                         static_cast<uint32_t>(i + 1)},
                                std::to_string(i));
        }

        SourceManager sm(tokens);
        REQUIRE(sm.size() == count);

        std::uniform_int_distribution<uint32_t> valid_idx_dist(0, count - 1);
        for (int i = 0; i < 100; ++i) {
            uint32_t idx = valid_idx_dist(rng);
            REQUIRE_NOTHROW(sm.get(idx));
            CHECK(sm.get(idx).get_lexeme() == std::to_string(idx));
        }

        REQUIRE_THROWS_AS(sm.get(count), std::out_of_range);
        REQUIRE_THROWS_AS(sm.get(count + 1000), std::out_of_range);
    }
}

TEST_CASE("Core: Logger multithreaded concurrency stress test",
          "[core][logger]") {
    Logger            logger;
    const std::string log_file = "stress_test.log";
    if (std::filesystem::exists(log_file)) {
        std::filesystem::remove(log_file);
    }

    logger.enable_log_file(log_file);

    const int                num_threads = 8;
    const int                writes_per_thread = 150;
    std::vector<std::thread> workers;

    for (int t = 0; t < num_threads; ++t) {
        workers.emplace_back([&logger, t, writes_per_thread]() {
            for (int i = 0; i < writes_per_thread; ++i) {
                logger.log(Logger::INFO, "Thread-", t, " event-", i);
            }
        });
    }

    for (auto& th : workers) {
        th.join();
    }

    std::ifstream file(log_file);
    REQUIRE(file.is_open());

    size_t      line_count = 0;
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty()) {
            line_count++;
            CHECK(line.find("[INFO]") != std::string::npos);
            CHECK(line.find("Thread-") != std::string::npos);
        }
    }

    REQUIRE(line_count == num_threads * writes_per_thread);

    file.close();
    std::filesystem::remove(log_file);
}
