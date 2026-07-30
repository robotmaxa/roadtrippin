#!/usr/bin/env python3
"""Regenerates code/*.js from the raw files in source/.

Run from the site root:  python3 tools/embed.py

To update code on the website: replace the file in source/<implementation>/,
run this script, commit, push. That's it.
"""
import json
import os

SITE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# Which files appear in each implementation's code browser, in display order.
MANIFEST = {
    "original": [
        "roadtrip.cpp", "roadtrip.hpp", "checker.cpp", "gen.cpp", "tests.cpp",
        "Makefile", "SPEC.md", "README.md",
        "western-usa.txt", "trip14.txt",
        "linear.txt", "choice.txt", "multisite.txt", "siteless.txt",
        "tie.txt", "unreachable.txt", "badheader.txt",
    ],
    "claude-code": [
        "Planner.cpp", "Planner.hpp", "Place.cpp", "Place.hpp",
        "Dataset.cpp", "Dataset.hpp", "main.cpp", "test_planner.cpp",
    ],
    "codex": [
        "planner.cpp", "planner.hpp", "main.cpp", "test_planner.cpp",
        "CMakeLists.txt", "original-codex.txt",
    ],
    "copilot": [
        "main.cpp",
    ],
    "claude-code-2": [
        "main.cpp", "RouteOptimizer.hpp", "LocationDatabase.hpp",
        "Location.hpp", "Geo.hpp", "locations.csv", "CMakeLists.txt", "README.md",
    ],
    "codex-2": [
        "main.cpp", "CMakeLists.txt", "README.md", "road_trip_data.txt",
    ],
    "copilot-2": [
        "main.cpp", "western-usa.txt",
    ],
}

LANG = {
    ".cpp": "cpp", ".hpp": "cpp", ".txt": "text",
    ".md": "markdown", "Makefile": "make", "CMakeLists.txt": "cmake",
}


def lang_for(name):
    if name in LANG:
        return LANG[name]
    _, ext = os.path.splitext(name)
    return LANG.get(ext, "text")


def main():
    os.makedirs(os.path.join(SITE, "code"), exist_ok=True)
    for impl, files in MANIFEST.items():
        entries = []
        for name in files:
            path = os.path.join(SITE, "source", impl, name)
            if not os.path.exists(path):
                print(f"  skip (missing): {impl}/{name}")
                continue
            with open(path, "r", encoding="utf-8", errors="replace") as f:
                content = f.read()
            entries.append({
                "name": name,
                "lang": lang_for(name),
                "lines": content.count("\n") + 1,
                "content": content,
            })
        out = (
            "window.SITE_CODE = window.SITE_CODE || {};\n"
            f"window.SITE_CODE[{json.dumps(impl)}] = {json.dumps(entries, indent=1)};\n"
        )
        dest = os.path.join(SITE, "code", f"{impl}.js")
        with open(dest, "w", encoding="utf-8") as f:
            f.write(out)
        print(f"wrote code/{impl}.js  ({len(entries)} files)")


if __name__ == "__main__":
    main()
