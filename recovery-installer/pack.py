#!/usr/bin/env python3
from pathlib import Path
import argparse, hashlib, shutil, zipfile

ap=argparse.ArgumentParser()
ap.add_argument('--helper',required=True,type=Path)
ap.add_argument('--templates',required=True,type=Path)
ap.add_argument('--dist',required=True,type=Path)
ap.add_argument('--version',required=True)
a=ap.parse_args()
a.dist.mkdir(parents=True,exist_ok=True)

def package(kind, filename):
    src=a.templates/kind
    stage=a.dist/f'.stage-{kind}'
    if stage.exists(): shutil.rmtree(stage)
    shutil.copytree(src,stage)
    (stage/'bin').mkdir(exist_ok=True)
    shutil.copy2(a.helper,stage/'bin/rodin-gpt')
    out=a.dist/filename
    with zipfile.ZipFile(out,'w',zipfile.ZIP_DEFLATED) as z:
        for p in sorted(stage.rglob('*')):
            if not p.is_file(): continue
            rel=p.relative_to(stage).as_posix()
            info=zipfile.ZipInfo(rel)
            info.create_system=3
            info.compress_type=zipfile.ZIP_DEFLATED
            mode=0o755 if rel.endswith('update-binary') or rel=='bin/rodin-gpt' else 0o644
            info.external_attr=(mode&0xffff)<<16
            z.writestr(info,p.read_bytes())
    shutil.rmtree(stage)
    return out

v=a.version.lstrip('v')
expand=package('expand',f'RODIN-Super-Expand-20G-v{v}.zip')
restore=package('restore',f'RODIN-Super-Restore-11G-v{v}.zip')
lines=[]
for p in [expand,restore]:
    lines.append(f'{hashlib.sha256(p.read_bytes()).hexdigest()}  {p.name}')
(a.dist/'SHA256SUMS.txt').write_text('\n'.join(lines)+'\n')
print('\n'.join(lines))
