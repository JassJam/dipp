add_rules("mode.debug")

--

if is_mode("check") then 
    set_policy("build.sanitizer.address", true)
    set_runtimes("MTd")
else 
    set_runtimes(is_mode("debug") and "MDd" or "MD")
end

set_languages("c++23")

add_extrafiles(".clang-format")
add_extrafiles(".clang-tidy")

if is_mode("debug") or is_mode("check") then
    add_defines("IMAGEPP_DEBUG")
end

includes("project/deps.lua")
includes("project/project.lua")
includes("project/tests.lua")