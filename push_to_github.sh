#!/bin/bash
# Run inside the CacheSim folder. Creates nothing on GitHub for you:
# first make an EMPTY repo at https://github.com/samisscam/CacheSim (no README/license), then run this.
set -e
git remote remove origin 2>/dev/null || true
git remote add origin https://github.com/samisscam/CacheSim.git
git push -u origin main develop
git push origin --all
git push origin --tags
echo "Done. Set 'develop' as default branch if you want PRs to target it by default."
