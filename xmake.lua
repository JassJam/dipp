option("test")
    set_default(false)
option_end()

option("benchmark")
    set_default(false)
option_end()

add_rules("mode.debug", "mode.release")
set_runtimes(is_mode("debug") and "MDd" or "MD")

--

set_languages("c++23")

add_extrafiles(".clang-format")
add_extrafiles(".clang-tidy")

if is_mode("debug") or is_mode("check") then
    add_defines("IMAGEPP_DEBUG")
end

includes("project/project.lua")

if is_config("test", true) then
    includes("project/tests.lua")
end

if is_config("benchmark", true) then
    includes("project/benchmarks.lua")
end