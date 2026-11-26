#pragma once

#include <array>
#include <sstream>

#include "../../codegen/include/chunk.hpp"

#define REG_AMOUNT 32

class VM {
   public:
    VM();
    VM(Chunk chunk);

    void load(Chunk chunk);
    void enable_logging(std::ostream* out);
    void disable_logging() noexcept;

    void run();  // executing chunk

   private:
    std::array<int32_t, REG_AMOUNT> regs;
    size_t                          cur_ins = 0;
    std::ostream*                   out;

    Chunk chunk;

   private:
    template <typename... targs>
    void log(const targs&... args) noexcept {
        if (out) {
            std::ostringstream ss;
            (ss << ... << args);

            static const char* next = "\n";
            *out << ss.str() << next;
        }
    }
};
