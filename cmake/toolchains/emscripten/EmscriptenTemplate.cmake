include_guard(DIRECTORY)
include(GetCurrentCompiler)

#
# create an Emscripten web page template
# usage:
# create_emscripten_html_template(OUTPUT_FILE 
#     [TITLE "Page Title"]              # HTML page title
#     [CANVAS_ID "canvas"]              # Canvas element ID
#     [TEMPLATE_FILE path/to/template]  # Custom template file to use instead of default
# )
# 
# Template file format:
# - Use {{TITLE}} for page title substitution
# - Use {{CANVAS_ID}} for canvas element ID substitution
# - Include {{{ SCRIPT }}} where Emscripten should inject the generated JavaScript
#
function(create_emscripten_html_template output_file)
    cmake_parse_arguments(ARG "" "TITLE;CANVAS_ID;TEMPLATE_FILE" "" ${ARGN})

    if (NOT ARG_TITLE)
        set(ARG_TITLE "WebAssembly Application")
    endif ()

    if (NOT ARG_CANVAS_ID)
        set(ARG_CANVAS_ID "canvas")
    endif ()

    # Check if custom template file is provided and exists
    if (ARG_TEMPLATE_FILE AND EXISTS "${ARG_TEMPLATE_FILE}")
        message(STATUS "Using custom HTML template: ${ARG_TEMPLATE_FILE}")

        # Read the template file
        file(READ "${ARG_TEMPLATE_FILE}" HTML_CONTENT)

        # Perform variable substitutions
        string(REPLACE "{{TITLE}}" "${ARG_TITLE}" HTML_CONTENT "${HTML_CONTENT}")
        string(REPLACE "{{CANVAS_ID}}" "${ARG_CANVAS_ID}" HTML_CONTENT "${HTML_CONTENT}")

        # Validate that the template has the required {{{ SCRIPT }}} placeholder
        if (NOT HTML_CONTENT MATCHES "\\{\\{\\{ SCRIPT \\}\\}\\}")
            message(WARNING "Custom template file '${ARG_TEMPLATE_FILE}' does not contain '{{{ SCRIPT }}}' placeholder. Emscripten may not work properly.")
        endif ()

    else ()
        # Use default template if no custom template provided or file doesn't exist
        if (ARG_TEMPLATE_FILE)
            message(WARNING "Custom template file '${ARG_TEMPLATE_FILE}' not found. Using default template.")
        endif ()

        _create_default_emscripten_template(HTML_CONTENT "${ARG_TITLE}" "${ARG_CANVAS_ID}")
    endif ()

    # Write the final HTML content
    file(WRITE "${output_file}" "${HTML_CONTENT}")
    message(STATUS "Created Emscripten HTML template: ${output_file}")
endfunction()

# Internal function to create the default HTML template
function(_create_default_emscripten_template output_var title canvas_id)
    set(HTML_CONTENT "<!DOCTYPE html>
<html lang=\"en\">
<head>
    <meta charset=\"UTF-8\">
    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">
    <title>${title}</title>
    <style>
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            margin: 0;
            padding: 20px;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            color: #fff;
        }
        .container {
            max-width: 1200px;
            margin: 0 auto;
            text-align: center;
        }
        h1 {
            margin-bottom: 30px;
            text-shadow: 2px 2px 4px rgba(0,0,0,0.3);
        }
        #${canvas_id} {
            border: 2px solid #fff;
            border-radius: 8px;
            background-color: #000;
            display: block;
            margin: 20px auto;
            box-shadow: 0 8px 32px rgba(0,0,0,0.2);
        }
        .controls {
            margin: 20px 0;
        }
        button {
            padding: 12px 24px;
            margin: 8px;
            background: rgba(255,255,255,0.9);
            color: #333;
            border: none;
            border-radius: 25px;
            cursor: pointer;
            font-weight: 600;
            transition: all 0.3s ease;
            box-shadow: 0 4px 15px rgba(0,0,0,0.2);
        }
        button:hover {
            background: #fff;
            transform: translateY(-2px);
            box-shadow: 0 6px 20px rgba(0,0,0,0.3);
        }
        .output {
            background: rgba(0,0,0,0.8);
            color: #00ff41;
            padding: 20px;
            text-align: left;
            font-family: 'Courier New', monospace;
            height: 200px;
            overflow-y: auto;
            margin: 20px 0;
            border-radius: 8px;
            border: 1px solid rgba(255,255,255,0.2);
            backdrop-filter: blur(10px);
        }
        .status {
            margin: 20px 0;
            padding: 10px;
            border-radius: 5px;
            background: rgba(255,255,255,0.1);
        }
        .loading { color: #ffd700; }
        .ready { color: #00ff41; }
        .error { color: #ff6b6b; }
    </style>
</head>
<body>
    <div class=\"container\">
        <h1>${title}</h1>
        <div id=\"status\" class=\"status loading\">Loading WebAssembly...</div>
        <canvas id=\"${canvas_id}\" width=\"800\" height=\"600\"></canvas>
        <div class=\"controls\">
            <button onclick=\"Module.requestFullscreen()\">Fullscreen</button>
            <button onclick=\"document.getElementById('output').innerHTML = ''\">Clear Console</button>
        </div>
        <div id=\"output\" class=\"output\"></div>
    </div>
    
    <script>
        var Module = {
            canvas: document.getElementById('${canvas_id}'),
            print: function(text) {
                var output = document.getElementById('output');
                output.innerHTML += text + '\\n';
                output.scrollTop = output.scrollHeight;
            },
            printErr: function(text) {
                var output = document.getElementById('output');
                output.innerHTML += '<span style=\"color: #ff6b6b;\">' + text + '</span>\\n';
                output.scrollTop = output.scrollHeight;
            },
            onRuntimeInitialized: function() {
                document.getElementById('status').innerHTML = 'WebAssembly Ready';
                document.getElementById('status').className = 'status ready';
            },
            onAbort: function(what) {
                document.getElementById('status').innerHTML = 'WebAssembly Error: ' + what;
                document.getElementById('status').className = 'status error';
            }
        };
    </script>
    {{{ SCRIPT }}}
</body>
</html>")

    set(${output_var} "${HTML_CONTENT}" PARENT_SCOPE)
endfunction()
