#!/usr/bin/env bash
# push the committed tree to stevejobs (ssh mini) and build it there in ~/shoal
set -euo pipefail
cd "$(dirname "$0")/.."
tar_path="${TMPDIR:-/tmp}/shoal-mini.tar"
git archive --format=tar HEAD > "$tar_path"
scp -q "$tar_path" mini:~/shoal.tar
ssh mini 'rm -rf ~/shoal/src ~/shoal/tests && mkdir -p ~/shoal && tar -xf ~/shoal.tar -C ~/shoal && cd ~/shoal && export PATH=$HOME/Library/Python/3.9/bin:$PATH && cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DSHOAL_NATIVE=ON -DSHOAL_VIEWER=OFF > build-cfg.log && ninja -C build > build.log && ./build/shoal_tests | tail -1'
