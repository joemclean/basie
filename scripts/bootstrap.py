#!/usr/bin/env python3
"""Fetch exact SDK revisions; never reset or overwrite a modified SDK checkout."""
import json
from pathlib import Path
import subprocess
import sys

root = Path(__file__).resolve().parents[1]
lock = json.loads((root/'dependencies.json').read_text())
for name, spec in lock.items():
    target = root/'vendor'/name
    if not target.exists():
        target.mkdir(parents=True)
        subprocess.run(['git', 'init', str(target)], check=True)
        subprocess.run(['git', '-C', str(target), 'remote', 'add', 'origin', spec['url']], check=True)
    elif not (target/'.git').exists():
        sys.exit(f'{target} is not a git checkout; move it aside before fetching dependencies.')
    def git(*arguments, capture=False):
        result = subprocess.run(['git', '-C', str(target), *arguments], check=True,
                                text=True, stdout=subprocess.PIPE if capture else None)
        return result.stdout.strip() if capture else None
    if git('status', '--porcelain', capture=True):
        sys.exit(f'{target} has local changes; preserve them before running bootstrap.')
    current = subprocess.run(['git', '-C', str(target), 'rev-parse', 'HEAD'], text=True, capture_output=True)
    if current.returncode or current.stdout.strip() != spec['revision']:
        git('-c', 'http.version=HTTP/1.1', 'fetch', '--depth=1', 'origin', spec['revision'])
        git('checkout', '--detach', spec['revision'])
    if spec['submodules']:
        git('-c', 'http.version=HTTP/1.1', 'submodule', 'update', '--init', '--depth=1', '--jobs=4', '--', *spec['submodules'])
    print(f'{name}: {spec["release"]} ({spec["revision"]})', flush=True)
