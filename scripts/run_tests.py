#!/usr/bin/env python3
"""Build host tests against the real core and application; no Daisy SDK needed."""
import argparse
import os
from pathlib import Path
import shlex
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument('--sanitize', action='store_true')
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
build = root / 'build-host' / ('sanitize' if args.sanitize else 'normal')
build.mkdir(parents=True, exist_ok=True)
compiler = shlex.split(os.environ.get('HOST_CXX', os.environ.get('CXX', 'c++')))
flags = ['-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-I'+str(root/'src')]
if args.sanitize:
    flags += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
core = ['theory', 'song', 'playback', 'quantizer', 'MIDI']
env = dict(os.environ, UBSAN_OPTIONS='halt_on_error=1')
for name, sources, extras in [
    ('core_tests', [root/'src'/(stem+'.cpp') for stem in core], []),
    ('integration_tests', sorted((root/'src').glob('*.cpp')), ['-I'+str(root/'tests/stubs')]),
]:
    binary = build/name
    subprocess.run(compiler+flags+extras+[str(p) for p in sources]+[str(root/'tests'/(name+'.cpp')), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True, env=env)
# A header must declare all its requirements even when included first.
for header in sorted((root/'src').glob('*.hpp')):
    subprocess.run(compiler+flags+['-I'+str(root/'tests/stubs'), '-x', 'c++', '-fsyntax-only', '-'],
                   input='#include "'+header.name+'"\n', text=True, check=True)
print('Standalone header checks passed', flush=True)
