#pragma once

#include <array>
#include <sstream>
#include <stack>

#include "../../codegen/include/chunk.hpp"

#define REG_AMOUNT 32
#define ENTRY_POINT "main"

class VM {
   public:
    VM();
    VM(Chunk chunk);

    void load(Chunk chunk);
    void enable_logging(std::ostream* out);
    void disable_logging() noexcept;

    void run();  // executing chunk

   private:
    enum class Func { NONE, HAI, MAIN };

   private:
    std::array<int32_t, REG_AMOUNT> regs;
    size_t                          cur_ins = 0;
    std::ostream*                   out;

    Chunk chunk;

    Func               cur_func;
    size_t             func_nesting;  // 0 -> global shit
                                      // 1 -> main
                                      // 2...inf -> another funcs
    std::stack<size_t> saved_ins;

    std::any ret_value;

   private:
    /*
     * pipeline idea:
     * > vm starts;
     * > 1. @crawling() -> vm scans through all instruction trying to find entry
     *                  point and skipping all funcs defs and global variables
     *                  defs;
     *                  when the vm finds entry point, it goes to @execute()
     *                  func and makes func_nestring = 1;
     * > 2. @execute() -> vm starts to execute entry point, goes through
     *                  instruction vector and executes all intructions
     *                  when the vm meets function calling, it goes into it,
     *                  making func_nesting++;
     *                  executing works until the vm meets RET in the @main func
     *                  or byte code bounds;
     * > 3. @out_exit_code() -> getting the value of @return operation in the
     *                  entry point, outputs it if logging is enabled
     * > vm sayonara;
     */
    void crawling();
    void execute();
    void out_exit_code();

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
