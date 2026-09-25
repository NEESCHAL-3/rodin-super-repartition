#!/usr/bin/env python3
import json
from pathlib import Path
p=json.loads((Path(__file__).resolve().parents[1]/'profiles'/'rodin.json').read_text())
assert p['sector_size']==4096
assert p['disk_bytes']==264_484_421_632
assert p['disk_lbas']==64_571_392
stock={k:tuple(v['stock']) for k,v in p['partitions'].items()}
dev={k:tuple(v['development']) for k,v in p['partitions'].items()}
order=['super','ffu','mem','countrycode_a','countrycode_b','oops','charger','rescue','blackbox','userdata','flashinfo']
for L in (stock,dev):
    for a,b in zip(order,order[1:]): assert L[a][1]+1==L[b][0],(a,b)
assert (stock['super'][1]-stock['super'][0]+1)*4096==11*1024**3
assert (dev['super'][1]-dev['super'][0]+1)*4096==20*1024**3
shift=dev['ffu'][0]-stock['ffu'][0]
assert shift==2_359_296 and shift*4096==9*1024**3
for n in ['ffu','mem','countrycode_a','countrycode_b','oops','charger','rescue','blackbox']:
    assert dev[n][0]-stock[n][0]==shift and dev[n][1]-stock[n][1]==shift
assert dev['super'][0]==stock['super'][0]
assert dev['userdata'][1]==stock['userdata'][1]
assert dev['flashinfo']==stock['flashinfo']
assert (dev['userdata'][1]-dev['userdata'][0]+1)*4096==240_506_601_472
print('rodin GPT geometry: OK')
