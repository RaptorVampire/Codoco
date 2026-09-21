/* Codoco - language database
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 * https://github.com/RaptorVampire/Codoco
 *
 * Every struct field is initialized explicitly:
 *   { name, exts[], line[], block_start, block_end, ascii_only }
 */
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "languages.h"
#include "utils.h"

const Language CODOCO_LANGS[] = {
    { "C"                     , {"c", "h"}, {"//"}, "/*", "*/", 0 },
    { "C++"                   , {"cpp", "cxx", "cc", "c++", "hpp", "hxx", "hh", "h++", "inl", "ipp", "tcc", "cppm", "ixx"}, {"//"}, "/*", "*/", 0 },
    { "C#"                    , {"cs", "csx"}, {"//"}, "/*", "*/", 0 },
    { "Objective-C"           , {"m"}, {"//"}, "/*", "*/", 0 },
    { "Objective-C++"         , {"mm"}, {"//"}, "/*", "*/", 0 },
    { "CUDA"                  , {"cu", "cuh"}, {"//"}, "/*", "*/", 0 },
    { "OpenCL"                , {"cl"}, {"//"}, "/*", "*/", 0 },
    { "GLSL"                  , {"glsl", "vert", "frag", "geom", "comp", "tesc", "tese"}, {"//"}, "/*", "*/", 0 },
    { "HLSL"                  , {"hlsl", "fx", "fxh"}, {"//"}, "/*", "*/", 0 },
    { "SystemVerilog"         , {"sv", "svh"}, {"//"}, "/*", "*/", 0 },
    { "Verilog"               , {"v", "vh"}, {"//"}, "/*", "*/", 0 },
    { "VHDL"                  , {"vhd", "vhdl"}, {"--"}, NULL, NULL, 0 },
    { "Java"                  , {"java"}, {"//"}, "/*", "*/", 0 },
    { "Kotlin"                , {"kt", "kts"}, {"//"}, "/*", "*/", 0 },
    { "Scala"                 , {"scala", "sc"}, {"//"}, "/*", "*/", 0 },
    { "Groovy"                , {"groovy", "gvy", "gradle"}, {"//"}, "/*", "*/", 0 },
    { "Clojure"               , {"clj", "cljs", "cljc", "edn"}, {";"}, NULL, NULL, 0 },
    { "JavaScript"            , {"js", "mjs", "cjs", "jsx"}, {"//"}, "/*", "*/", 0 },
    { "TypeScript"            , {"ts", "tsx", "mts", "cts"}, {"//"}, "/*", "*/", 0 },
    { "CoffeeScript"          , {"coffee", "litcoffee"}, {"#"}, "###", "###", 0 },
    { "Elm"                   , {"elm"}, {"--"}, "{-", "-}", 0 },
    { "PureScript"            , {"purs"}, {"--"}, "{-", "-}", 0 },
    { "ReasonML"              , {"re", "rei"}, {"//"}, "/*", "*/", 0 },
    { "Dart"                  , {"dart"}, {"//"}, "/*", "*/", 0 },
    { "ActionScript"          , {"as"}, {"//"}, "/*", "*/", 0 },
    { "Haxe"                  , {"hx", "hxml"}, {"//"}, "/*", "*/", 0 },
    { "Python"                , {"py", "pyw", "pyi", "pyx", "pxd", "pxi", "scons", "wsgi"}, {"#"}, NULL, NULL, 0 },
    { "Cython"                , {"pyx", "pxd", "pxi"}, {"#"}, NULL, NULL, 0 },
    { "Ruby"                  , {"rb", "rake", "gemspec", "ru", "podspec"}, {"#"}, "=begin", "=end", 0 },
    { "Crystal"               , {"cr"}, {"#"}, NULL, NULL, 0 },
    { "Nim"                   , {"nim", "nims", "nimble"}, {"#"}, NULL, NULL, 0 },
    { "Julia"                 , {"jl"}, {"#"}, "#=", "=#", 0 },
    { "Boo"                   , {"boo"}, {"//"}, "/*", "*/", 0 },
    { "Go"                    , {"go"}, {"//"}, "/*", "*/", 0 },
    { "Rust"                  , {"rs"}, {"//"}, "/*", "*/", 0 },
    { "Zig"                   , {"zig"}, {"//"}, NULL, NULL, 0 },
    { "D"                     , {"d", "di"}, {"//"}, "/*", "*/", 0 },
    { "V"                     , {"v"}, {"//"}, "/*", "*/", 0 },
    { "Vala"                  , {"vala", "vapi"}, {"//"}, "/*", "*/", 0 },
    { "Genie"                 , {"gs"}, {"//"}, "/*", "*/", 0 },
    { "Pascal"                , {"pas", "pp", "p"}, {"//"}, "{", "}", 0 },
    { "Delphi"                , {"dpr", "dpk", "dfm"}, {"//"}, "{", "}", 0 },
    { "Ada"                   , {"adb", "ads", "ada"}, {"--"}, NULL, NULL, 0 },
    { "Fortran"               , {"f", "for", "f77", "f90", "f95", "f03", "f08", "f15", "ftn"}, {"!"}, NULL, NULL, 0 },
    { "Assembly"              , {"asm", "s", "S"}, {";"}, NULL, NULL, 0 },
    { "NASM"                  , {"nasm"}, {";"}, NULL, NULL, 0 },
    { "Motorola 68K"          , {"s68"}, {";"}, NULL, NULL, 0 },
    { "RISC-V asm"            , {"rv"}, {"#"}, NULL, NULL, 0 },
    { "Shell"                 , {"sh", "ksh", "dash"}, {"#"}, NULL, NULL, 0 },
    { "Bash"                  , {"bash"}, {"#"}, NULL, NULL, 0 },
    { "Zsh"                   , {"zsh"}, {"#"}, NULL, NULL, 0 },
    { "Fish"                  , {"fish"}, {"#"}, NULL, NULL, 0 },
    { "PowerShell"            , {"ps1", "psm1", "psd1"}, {"#"}, "<#", "#>", 0 },
    { "Batch"                 , {"bat", "cmd"}, {"REM", "::"}, NULL, NULL, 0 },
    { "VBScript"              , {"vbs"}, {"'"}, NULL, NULL, 0 },
    { "Visual Basic"          , {"vb", "bas", "frm", "cls"}, {"'"}, NULL, NULL, 0 },
    { "AutoHotkey"            , {"ahk"}, {";"}, "/*", "*/", 0 },
    { "AutoIt"                , {"au3"}, {";"}, NULL, NULL, 0 },
    { "Haskell"               , {"hs", "lhs", "hsc"}, {"--"}, "{-", "-}", 0 },
    { "Erlang"                , {"erl", "hrl"}, {"%"}, NULL, NULL, 0 },
    { "Elixir"                , {"ex", "exs"}, {"#"}, NULL, NULL, 0 },
    { "Lisp"                  , {"lisp", "lsp", "cl"}, {";"}, "#|", "|#", 0 },
    { "Scheme"                , {"scm", "ss", "sld", "sls"}, {";"}, "#|", "|#", 0 },
    { "Racket"                , {"rkt", "rktl", "rktd"}, {";"}, "#|", "|#", 0 },
    { "OCaml"                 , {"ml", "mli", "mll", "mly"}, {"(*"}, "(*", "*)", 0 },
    { "Standard ML"           , {"sml", "sig", "fun"}, {"(*"}, "(*", "*)", 0 },
    { "F#"                    , {"fs", "fsi", "fsx"}, {"//"}, "(*", "*)", 0 },
    { "Idris"                 , {"idr", "lidr"}, {"--"}, "{-", "-}", 0 },
    { "Agda"                  , {"agda", "lagda"}, {"--"}, "{-", "-}", 0 },
    { "Coq"                   , {"v"}, {"(*"}, "(*", "*)", 0 },
    { "Lean"                  , {"lean"}, {"--"}, "/-", "-/", 0 },
    { "Isabelle"              , {"thy"}, {"--"}, "(*", "*)", 0 },
    { "Mercury"               , {"m"}, {"%"}, NULL, NULL, 0 },
    { "Prolog"                , {"pl", "pro", "prolog"}, {"%"}, "/*", "*/", 0 },
    { "Datalog"               , {"dl"}, {"%"}, NULL, NULL, 0 },
    { "R"                     , {"r", "Rmd"}, {"#"}, NULL, NULL, 0 },
    { "MATLAB"                , {"m"}, {"%"}, "%{", "%}", 0 },
    { "Octave"                , {"m"}, {"%"}, "%{", "%}", 0 },
    { "Scilab"                , {"sci", "sce"}, {"//"}, "/*", "*/", 0 },
    { "Wolfram"               , {"wl", "nb"}, {"(*"}, "(*", "*)", 0 },
    { "SAS"                   , {"sas"}, {"*"}, "/*", "*/", 0 },
    { "Stata"                 , {"do", "ado"}, {"*"}, "/*", "*/", 0 },
    { "SPSS"                  , {"sps"}, {"*"}, NULL, NULL, 0 },
    { "HTML"                  , {"html", "htm", "xhtml", "shtml"}, {NULL}, "<!--", "-->", 0 },
    { "XML"                   , {"xml", "xsd", "xsl", "xslt", "svg", "plist", "csproj", "vcxproj", "props", "targets", "resx"}, {NULL}, "<!--", "-->", 0 },
    { "CSS"                   , {"css"}, {NULL}, "/*", "*/", 0 },
    { "SCSS"                  , {"scss"}, {"//"}, "/*", "*/", 0 },
    { "SASS"                  , {"sass"}, {"//"}, "/*", "*/", 0 },
    { "LESS"                  , {"less"}, {"//"}, "/*", "*/", 0 },
    { "Stylus"                , {"styl"}, {"//"}, "/*", "*/", 0 },
    { "JSON"                  , {"json", "jsonc", "geojson", "topojson"}, {"//"}, "/*", "*/", 0 },
    { "JSON5"                 , {"json5"}, {"//"}, "/*", "*/", 0 },
    { "JSONNet"               , {"jsonnet", "libsonnet"}, {"//"}, "/*", "*/", 0 },
    { "YAML"                  , {"yaml", "yml"}, {"#"}, NULL, NULL, 0 },
    { "TOML"                  , {"toml"}, {"#"}, NULL, NULL, 0 },
    { "INI"                   , {"ini", "cfg", "conf", "properties"}, {";", "#"}, NULL, NULL, 0 },
    { "Markdown"              , {"md", "markdown", "mdown", "mkd"}, {NULL}, "<!--", "-->", 0 },
    { "reStructuredText"      , {"rst"}, {NULL}, NULL, NULL, 0 },
    { "AsciiDoc"              , {"adoc", "asciidoc"}, {NULL}, NULL, NULL, 0 },
    { "LaTeX"                 , {"tex", "sty", "cls", "ltx"}, {"%"}, NULL, NULL, 0 },
    { "BibTeX"                , {"bib"}, {"%"}, NULL, NULL, 0 },
    { "WikiText"              , {"wiki", "mediawiki"}, {NULL}, "<!--", "-->", 0 },
    { "SQL"                   , {"sql"}, {"--"}, "/*", "*/", 0 },
    { "PL/SQL"                , {"pls", "plsql", "pkb", "pks", "prc", "fnc", "trg"}, {"--"}, "/*", "*/", 0 },
    { "T-SQL"                 , {"tsql"}, {"--"}, "/*", "*/", 0 },
    { "MySQL"                 , {"mysql"}, {"--", "#"}, "/*", "*/", 0 },
    { "PostgreSQL"            , {"pgsql"}, {"--"}, "/*", "*/", 0 },
    { "Cypher"                , {"cql", "cypher"}, {"//"}, "/*", "*/", 0 },
    { "GraphQL"               , {"graphql", "gql"}, {"#"}, NULL, NULL, 0 },
    { "Makefile"              , {"mk", "mak", "make"}, {"#"}, NULL, NULL, 0 },
    { "CMake"                 , {"cmake"}, {"#"}, "#[[", "]]", 0 },
    { "Meson"                 , {"meson"}, {"#"}, NULL, NULL, 0 },
    { "Bazel"                 , {"bzl", "bazel"}, {"#"}, NULL, NULL, 0 },
    { "Gradle"                , {"gradle"}, {"//"}, "/*", "*/", 0 },
    { "Dockerfile"            , {"dockerfile"}, {"#"}, NULL, NULL, 0 },
    { "Containerfile"         , {"containerfile"}, {"#"}, NULL, NULL, 0 },
    { "Nix"                   , {"nix"}, {"#"}, "/*", "*/", 0 },
    { "Terraform"             , {"tf", "tfvars"}, {"#", "//"}, "/*", "*/", 0 },
    { "HCL"                   , {"hcl"}, {"#", "//"}, "/*", "*/", 0 },
    { "Puppet"                , {"pp"}, {"#"}, "/*", "*/", 0 },
    { "Protocol Buffers"      , {"proto"}, {"//"}, "/*", "*/", 0 },
    { "Thrift"                , {"thrift"}, {"//", "#"}, "/*", "*/", 0 },
    { "Cap'n Proto"           , {"capnp"}, {"#"}, NULL, NULL, 0 },
    { "FlatBuffers"           , {"fbs"}, {"//"}, "/*", "*/", 0 },
    { "ASN.1"                 , {"asn", "asn1"}, {"--"}, NULL, NULL, 0 },
    { "WebIDL"                , {"widl", "idl"}, {"//"}, "/*", "*/", 0 },
    { "LLVM IR"               , {"ll"}, {";"}, NULL, NULL, 0 },
    { "WebAssembly"           , {"wat", "wast"}, {";;"}, "(;", ";)", 0 },
    { "COBOL"                 , {"cob", "cbl", "cpy"}, {"*"}, NULL, NULL, 0 },
    { "BASIC"                 , {"bas"}, {"'"}, NULL, NULL, 0 },
    { "QBasic"                , {"qb"}, {"'"}, NULL, NULL, 0 },
    { "FreeBASIC"             , {"bas", "bi"}, {"'"}, NULL, NULL, 0 },
    { "Pascal Script"         , {"psc"}, {"//"}, "(*", "*)", 0 },
    { "Smalltalk"             , {"st"}, {NULL}, "\"", "\"", 0 },
    { "Forth"                 , {"fth", "4th", "forth"}, {"\\"}, "( ", " )", 0 },
    { "Tcl"                   , {"tcl"}, {"#"}, NULL, NULL, 0 },
    { "AWK"                   , {"awk"}, {"#"}, NULL, NULL, 0 },
    { "Sed"                   , {"sed"}, {"#"}, NULL, NULL, 0 },
    { "Perl"                  , {"pl", "pm", "t", "pod", "psgi"}, {"#"}, "=pod", "=cut", 0 },
    { "Raku"                  , {"p6", "pl6", "pm6", "raku", "rakumod"}, {"#"}, "=begin", "=end", 0 },
    { "Lua"                   , {"lua"}, {"--"}, "--[[", "]]", 0 },
    { "MoonScript"            , {"moon"}, {"--"}, "--[[", "]]", 0 },
    { "Nemerle"               , {"n"}, {"//"}, "/*", "*/", 0 },
    { "Chapel"                , {"chpl"}, {"//"}, "/*", "*/", 0 },
    { "X10"                   , {"x10"}, {"//"}, "/*", "*/", 0 },
    { "ABAP"                  , {"abap"}, {"*"}, NULL, NULL, 0 },
    { "Solidity"              , {"sol"}, {"//"}, "/*", "*/", 0 },
    { "Vyper"                 , {"vy"}, {"#"}, NULL, NULL, 0 },
    { "Move"                  , {"move"}, {"//"}, "/*", "*/", 0 },
    { "Cairo"                 , {"cairo"}, {"//"}, NULL, NULL, 0 },
    { "Jinja2"                , {"j2", "jinja", "jinja2"}, {"{#"}, "{#", "#}", 0 },
    { "Handlebars"            , {"hbs", "handlebars"}, {"{{!"}, "{{!", "}}", 0 },
    { "Mustache"              , {"mustache"}, {"{{!"}, "{{!", "}}", 0 },
    { "ERB"                   , {"erb", "rhtml"}, {"<%#"}, "<%#", "%>", 0 },
    { "PHP"                   , {"php", "php3", "php4", "php5", "phtml", "inc"}, {"//", "#"}, "/*", "*/", 0 },
    { "Pug"                   , {"pug", "jade"}, {"//-"}, NULL, NULL, 0 },
    { "Haml"                  , {"haml"}, {"-#"}, NULL, NULL, 0 },
    { "Slim"                  , {"slim"}, {"/"}, NULL, NULL, 0 },
    { "EJS"                   , {"ejs"}, {"<%#"}, "<%#", "%>", 0 },
    { "Twig"                  , {"twig"}, {"{#"}, "{#", "#}", 0 },
    { "Cisco IOS"             , {"ios"}, {"!"}, NULL, NULL, 0 },
    { "NetLinx"               , {"axs", "axi"}, {"//"}, "(*", "*)", 0 },
    { "AppleScript"           , {"applescript", "scpt"}, {"--"}, "(*", "*)", 0 },
    { "Pike"                  , {"pike", "pmod"}, {"//"}, "/*", "*/", 0 },
    { "Io"                    , {"io"}, {"//", "#"}, "/*", "*/", 0 },
    { "J"                     , {"ijs"}, {"NB."}, NULL, NULL, 0 },
    { "K"                     , {"k"}, {"/"}, NULL, NULL, 0 },
    { "Q"                     , {"q"}, {"/"}, NULL, NULL, 0 },
    { "Hy"                    , {"hy"}, {";"}, NULL, NULL, 0 },
    { "Janet"                 , {"janet"}, {"#"}, NULL, NULL, 0 },
    { "Fennel"                , {"fnl"}, {";"}, NULL, NULL, 0 },
    { "Wren"                  , {"wren"}, {"//"}, "/*", "*/", 0 },
    { "Falcon"                , {"fal", "ftd"}, {"//", "#"}, "/*", "*/", 0 },
    { "Ring"                  , {"ring"}, {"#"}, NULL, NULL, 0 },
    { "Pony"                  , {"pony"}, {"//"}, "/*", "*/", 0 },
    { "Red"                   , {"red", "reds"}, {";"}, NULL, NULL, 0 },
    { "Rebol"                 , {"reb"}, {";"}, NULL, NULL, 0 },
    { "Factor"                , {"factor"}, {"!"}, "/*", "*/", 0 },
    { "UnrealScript"          , {"uc"}, {"//"}, "/*", "*/", 0 },
    { "AngelScript"           , {"as"}, {"//"}, "/*", "*/", 0 },
    { "Squirrel"              , {"nut"}, {"//"}, "/*", "*/", 0 },
    { "PAWN"                  , {"pwn"}, {"//"}, "/*", "*/", 0 },
    { "SourcePawn"            , {"sp"}, {"//"}, "/*", "*/", 0 },
    { "Pawn"                  , {"p"}, {"//"}, "/*", "*/", 0 },
    { "Text"                  , {"txt", "text"}, {NULL}, NULL, NULL, 0 },
    { "CSV"                   , {"csv", "tsv"}, {NULL}, NULL, NULL, 0 },
    { "Log"                   , {"log"}, {NULL}, NULL, NULL, 0 },
};

const int CODOCO_LANG_COUNT = (int)(sizeof(CODOCO_LANGS) / sizeof(CODOCO_LANGS[0]));

const Language *lang_by_extension(const char *ext) {
    if (!ext || !*ext) return NULL;
    for (int i = 0; i < CODOCO_LANG_COUNT; i++) {
        const Language *lg = &CODOCO_LANGS[i];
        for (int j = 0; j < LANG_MAX_EXT && lg->exts[j]; j++) {
            if (str_icmp(lg->exts[j], ext) == 0) return lg;
        }
    }
    return NULL;
}

const Language *lang_by_name(const char *name) {
    if (!name) return NULL;
    for (int i = 0; i < CODOCO_LANG_COUNT; i++) {
        if (str_icmp(CODOCO_LANGS[i].name, name) == 0) return &CODOCO_LANGS[i];
    }
    return NULL;
}

const Language *lang_by_shebang(const char *first_line) {
    if (!first_line || first_line[0] != '#' || first_line[1] != '!') return NULL;
    const char *p = first_line + 2;
    while (*p == ' ' || *p == '\t') p++;
    const char *end = p;
    while (*end && *end != ' ' && *end != '\t' && *end != '\n' && *end != '\r') end++;

    const char *base = p;
    const char *slash = NULL;
    for (const char *q = p; q < end; q++) if (*q == '/') slash = q;
    if (slash) base = slash + 1;

    struct { const char *key; const char *lang; } tbl[] = {
        {"python","Python"}, {"python2","Python"}, {"python3","Python"},
        {"ruby","Ruby"}, {"perl","Perl"}, {"node","JavaScript"}, {"nodejs","JavaScript"},
        {"bash","Bash"}, {"sh","Shell"}, {"zsh","Zsh"}, {"ksh","Shell"}, {"dash","Shell"},
        {"fish","Fish"}, {"lua","Lua"}, {"php","PHP"}, {"awk","AWK"}, {"gawk","AWK"},
        {"sed","Sed"}, {"tclsh","Tcl"}, {"wish","Tcl"}, {"escript","Erlang"},
        {"Rscript","R"}, {"groovy","Groovy"}, {"scala","Scala"}, {"julia","Julia"},
        {"raku","Raku"}, {"perl6","Raku"}, {"elixir","Elixir"}, {"deno","TypeScript"},
        {"bun","JavaScript"}, {"pwsh","PowerShell"}, {"powershell","PowerShell"},
        {"tcc","C"}, {"gcc","C"}, {"clang","C"}, {"cc","C"},
        {"g++","C++"}, {"clang++","C++"},
        {"cargo","Rust"}, {"rustc","Rust"}, {"go","Go"},
        {NULL,NULL}
    };
    for (int i = 0; tbl[i].key; i++) {
        size_t kl = strlen(tbl[i].key);
        if (strncmp(base, tbl[i].key, kl) == 0) {
            const char *rest = base + kl;
            if (*rest == 0 || *rest == ' ' || *rest == '\t' ||
                *rest == '\n' || *rest == '\r' ||
                (*rest >= '0' && *rest <= '9')) {
                return lang_by_name(tbl[i].lang);
            }
        }
    }
    return NULL;
}

int lang_supports_ext(const char *ext) {
    return lang_by_extension(ext) != NULL;
}

void lang_list_all(void) {
    printf("Supported languages (%d):\n", CODOCO_LANG_COUNT);
    for (int i = 0; i < CODOCO_LANG_COUNT; i++) {
        const Language *lg = &CODOCO_LANGS[i];
        printf("  %-22s", lg->name);
        int n = 0;
        for (int j = 0; j < LANG_MAX_EXT && lg->exts[j]; j++, n++) {
            if (n > 0) printf(",");
            printf(".%s", lg->exts[j]);
        }
        printf("\n");
    }
}



/* ------------------------------------------------------------------ *
 *  Filename rules: extension-less files (Makefile, Dockerfile,       *
 *  CMakeLists.txt, Rakefile, Gemfile, .gitignore, LICENSE, ...).     *
 * ------------------------------------------------------------------ */

typedef struct { const char *filename; const char *lang; } FilenameRule;

static const FilenameRule FILENAME_RULES[] = {
    /* --- build systems -------------------------------------------- */
    { "Makefile",              "Makefile" },
    { "makefile",              "Makefile" },
    { "GNUmakefile",           "Makefile" },
    { "BSDmakefile",           "Makefile" },
    { "Kbuild",                "Makefile" },
    { "Makefile.am",           "Makefile" },
    { "Makefile.in",           "Makefile" },
    { "CMakeLists.txt",        "CMake" },
    { "meson.build",           "Meson" },
    { "BUILD",                 "Bazel" },
    { "BUILD.bazel",           "Bazel" },
    { "WORKSPACE",             "Bazel" },
    { "configure",             "Shell" },
    { "configure.ac",          "Shell" },
    { "Makefile.PL",           "Perl" },

    /* --- containers / infra --------------------------------------- */
    { "Dockerfile",            "Dockerfile" },
    { "Containerfile",         "Containerfile" },
    { ".dockerignore",         "Text" },
    { "Vagrantfile",           "Ruby" },
    { "Procfile",              "Text" },
    { "Caddyfile",             "Text" },
    { "Jenkinsfile",           "Groovy" },
    { "Brewfile",              "Ruby" },
    { "Justfile",              "Makefile" },
    { "justfile",              "Makefile" },

    /* --- ruby ----------------------------------------------------- */
    { "Rakefile",              "Ruby" },
    { "Gemfile",               "Ruby" },
    { "Gemfile.lock",          "Ruby" },
    { "Guardfile",             "Ruby" },
    { "Capfile",               "Ruby" },
    { "Podfile",               "Ruby" },
    { "Podfile.lock",          "YAML" },
    { "Berksfile",             "Ruby" },
    { "Fastfile",              "Ruby" },
    { "Appfile",               "Ruby" },
    { "Deliverfile",           "Ruby" },
    { "Matchfile",             "Ruby" },

    /* --- python --------------------------------------------------- */
    { "SConstruct",            "Python" },
    { "SConscript",            "Python" },
    { "wscript",               "Python" },
    { "setup.py",              "Python" },
    { "pyproject.toml",        "TOML" },
    { "Pipfile",               "TOML" },
    { "Pipfile.lock",          "JSON" },
    { "setup.cfg",             "INI" },
    { "tox.ini",               "INI" },
    { "requirements.txt",      "Text" },
    { "requirements-dev.txt",  "Text" },
    { "MANIFEST.in",           "Text" },

    /* --- rust ----------------------------------------------------- */
    { "Cargo.toml",            "TOML" },
    { "Cargo.lock",            "TOML" },

    /* --- go ------------------------------------------------------- */
    { "go.mod",                "Go" },
    { "go.sum",                "Text" },

    /* --- node ----------------------------------------------------- */
    { "package.json",          "JSON" },
    { "package-lock.json",     "JSON" },
    { "yarn.lock",             "Text" },
    { "tsconfig.json",         "JSON" },
    { "tslint.json",           "JSON" },
    { "jsconfig.json",         "JSON" },
    { ".babelrc",              "JSON" },
    { ".eslintrc",             "JSON" },
    { ".eslintrc.json",        "JSON" },
    { ".prettierrc",           "JSON" },
    { ".prettierrc.json",      "JSON" },
    { ".npmrc",                "INI" },
    { ".nvmrc",                "Text" },
    { ".yarnrc",               "INI" },

    /* --- jvm ------------------------------------------------------ */
    { "build.gradle",          "Gradle" },
    { "build.gradle.kts",      "Gradle" },
    { "settings.gradle",       "Gradle" },
    { "settings.gradle.kts",   "Gradle" },
    { "gradlew",               "Shell" },
    { "gradlew.bat",           "Batch" },
    { "pom.xml",               "XML" },
    { "build.xml",             "XML" },

    /* --- yaml / config ------------------------------------------- */
    { "docker-compose.yml",    "YAML" },
    { "docker-compose.yaml",   "YAML" },
    { ".travis.yml",           "YAML" },
    { "travis.yml",            "YAML" },
    { ".gitlab-ci.yml",        "YAML" },
    { "appveyor.yml",          "YAML" },
    { "ansible.cfg",           "INI" },
    { "playbook.yml",          "YAML" },
    { "cloudbuild.yaml",       "YAML" },

    /* --- dotfiles ------------------------------------------------- */
    { ".gitignore",            "Text" },
    { ".gitattributes",        "Text" },
    { ".gitmodules",           "Text" },
    { ".gitkeep",              "Text" },
    { ".gitconfig",            "INI" },
    { ".editorconfig",         "INI" },
    { ".env",                  "INI" },
    { ".env.example",          "INI" },
    { ".htaccess",             "Text" },
    { ".htpasswd",             "Text" },
    { ".bashrc",               "Shell" },
    { ".bash_profile",         "Shell" },
    { ".profile",              "Shell" },
    { ".zshrc",                "Shell" },
    { ".vimrc",                "Text" },

    /* --- docs / meta --------------------------------------------- */
    { "README",                "Text" },
    { "README.txt",            "Text" },
    { "README.md",             "Markdown" },
    { "README.rst",            "reStructuredText" },
    { "LICENSE",               "Text" },
    { "LICENSE.txt",           "Text" },
    { "LICENSE.md",            "Markdown" },
    { "LICENCE",               "Text" },
    { "LICENCE.txt",           "Text" },
    { "COPYING",               "Text" },
    { "COPYRIGHT",             "Text" },
    { "NOTICE",                "Text" },
    { "AUTHORS",               "Text" },
    { "CONTRIBUTORS",          "Text" },
    { "CHANGELOG",             "Text" },
    { "CHANGELOG.md",          "Markdown" },
    { "CHANGES",               "Text" },
    { "HISTORY",               "Text" },
    { "INSTALL",               "Text" },
    { "NEWS",                  "Text" },
    { "TODO",                  "Text" },
    { "VERSION",               "Text" },
    { "CNAME",                 "Text" },
    { "MANIFEST",              "Text" },

    /* --- apple / android ----------------------------------------- */
    { "Cartfile",              "Text" },
    { "Cartfile.resolved",     "Text" },
    { "Info.plist",            "XML" },
    { "AndroidManifest.xml",   "XML" },

    /* --- terraform ----------------------------------------------- */
    { "terraform.tfvars",      "Terraform" },
    { ".terraform.lock.hcl",   "HCL" },

    /* --- erlang / elixir ----------------------------------------- */
    { "mix.exs",               "Elixir" },
    { "mix.lock",              "Elixir" },
    { "rebar.config",          "Erlang" },
    { "rebar.lock",            "Erlang" },

    /* --- haskell ------------------------------------------------- */
    { "stack.yaml",            "YAML" },
    { "cabal.project",         "Haskell" },

    /* --- php / composer ------------------------------------------ */
    { "composer.json",         "JSON" },
    { "composer.lock",         "JSON" },

    /* --- dart / flutter ------------------------------------------ */
    { "pubspec.yaml",          "YAML" },
    { "pubspec.lock",          "YAML" },

    /* --- swift --------------------------------------------------- */
    { "Package.swift",         "Swift" },
    { "Package.resolved",      "JSON" },

    /* --- lua / misc ---------------------------------------------- */
    { ".luacheckrc",           "Lua" },
    { ".clang-format",         "YAML" },
    { ".clang-tidy",           "YAML" },

    { NULL, NULL }
};

const Language *lang_by_filename(const char *filename) {
    if (!filename || !*filename) return NULL;
    for (int i = 0; FILENAME_RULES[i].filename; i++) {
        if (str_icmp(FILENAME_RULES[i].filename, filename) == 0) {
            return lang_by_name(FILENAME_RULES[i].lang);
        }
    }
    return NULL;
}

int lang_is_known_filename(const char *name) {
    return lang_by_filename(name) != NULL;
}
