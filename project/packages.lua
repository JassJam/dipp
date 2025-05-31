
local function install_benchmark_packages()
    if is_config("benchmark", true) then
        add_requires("benchmark")
        add_requires("kangaru")
        add_requires("fruit")
    end
end

local function install_result_packages()
    local boost_lib_version = "1.88.0"
    local boost_modules = {
    }

    if is_config("test", true) then
        table.insert(boost_modules, "test")
    end
    if is_config("error-type", "result") then
        table.insert(boost_modules, "leaf")
    end

    -- if boost_modules is empty, then we don't need to install boost
    if #boost_modules == 0 then
        return
    end

    table.insert(boost_modules, "regex")
    add_requires("boost[cmake,".. table.concat(boost_modules, ",") .."] " .. boost_lib_version)
end

install_result_packages()
install_benchmark_packages()

--

function add_result_packages()
    if not is_config("error-type", "result") then
        return
    end

    add_packages("boost", {public = true})
end
