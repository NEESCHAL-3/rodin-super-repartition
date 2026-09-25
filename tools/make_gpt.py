#!/usr/bin/env python3
from __future__ import annotations
import argparse, json, struct, uuid, zlib
from pathlib import Path

SIG=b"EFI PART"
def crc32(b:bytes)->int: return zlib.crc32(b)&0xffffffff

def header(buf:bytes, off:int, ss:int)->dict:
    s=buf[off:off+ss]
    if s[:8]!=SIG: raise SystemExit(f"GPT signature missing at offset {off}")
    rev,hsz,hcrc=struct.unpack_from('<III',s,8)
    if rev!=0x10000: raise SystemExit(f"Unexpected GPT revision 0x{rev:x}")
    cur,bak=struct.unpack_from('<QQ',s,24); first,last=struct.unpack_from('<QQ',s,40)
    guid=str(uuid.UUID(bytes_le=bytes(s[56:72])))
    ent=struct.unpack_from('<Q',s,72)[0]; n,esz,ecrc=struct.unpack_from('<III',s,80)
    t=bytearray(s[:hsz]); t[16:20]=b'\0'*4
    if crc32(bytes(t))!=hcrc: raise SystemExit(f"Header CRC mismatch at LBA {cur}")
    return dict(off=off,hsz=hsz,cur=cur,bak=bak,first=first,last=last,guid=guid,ent=ent,n=n,esz=esz,ecrc=ecrc)

def find_backup(tail:bytes, base:int, ss:int)->dict:
    for off in range(0,len(tail)-ss+1,ss):
        if tail[off:off+8]==SIG:
            h=header(tail,off,ss)
            if h['cur']==base+off//ss: return h
    raise SystemExit('Backup GPT header not found')

def entries(buf:bytes,h:dict,base:int,ss:int):
    exact=h['n']*h['esz']; sectors=(exact+ss-1)//ss; off=(h['ent']-base)*ss
    raw=bytearray(buf[off:off+sectors*ss])
    if len(raw)!=sectors*ss: raise SystemExit('Entry array outside supplied dump')
    if crc32(bytes(raw[:exact]))!=h['ecrc']: raise SystemExit('Entry-array CRC mismatch')
    return raw,exact,sectors

def decode(tab:bytes|bytearray,idx:int,esz:int)->dict:
    o=(idx-1)*esz; e=tab[o:o+esz]
    first,last,attrs=struct.unpack_from('<QQQ',e,32)
    return dict(name=bytes(e[56:128]).decode('utf-16le',errors='ignore').rstrip('\0'),first=first,last=last,attrs=attrs,
                type_guid=str(uuid.UUID(bytes_le=bytes(e[:16]))),unique_guid=str(uuid.UUID(bytes_le=bytes(e[16:32]))))

def patch_header(sec:bytes,h:dict,ecrc:int)->bytes:
    x=bytearray(sec); struct.pack_into('<I',x,88,ecrc); struct.pack_into('<I',x,16,0)
    struct.pack_into('<I',x,16,crc32(bytes(x[:h['hsz']])))
    return bytes(x)

def layout(p:dict,key:str): return {n:tuple(v[key]) for n,v in p['partitions'].items()}

def validate_contiguous(L:dict):
    order=['super','ffu','mem','countrycode_a','countrycode_b','oops','charger','rescue','blackbox','userdata','flashinfo']
    for a,b in zip(order,order[1:]):
        if L[a][1]+1!=L[b][0]: raise SystemExit(f'Gap/overlap between {a} and {b}')

def validate_stock(tab:bytearray,p:dict):
    for name,v in p['partitions'].items():
        cur=decode(tab,v['index'],p['gpt']['entry_size']); exp=tuple(v['stock'])
        if cur['name']!=name or (cur['first'],cur['last'])!=exp:
            raise SystemExit(f"REFUSING: {name} source geometry/name mismatch")

def patch_dev(tab:bytearray,p:dict):
    changes={}
    for name,v in p['partitions'].items():
        idx=v['index']; old=decode(tab,idx,p['gpt']['entry_size']); ns,ne=v['development']; o=(idx-1)*p['gpt']['entry_size']
        struct.pack_into('<QQ',tab,o+32,ns,ne); changes[name]={'before':old,'after':decode(tab,idx,p['gpt']['entry_size'])}
    return tab,changes

def write_set(d:Path,ph:bytes,pe:bytes,be:bytes,bh:bytes,m:dict):
    d.mkdir(parents=True,exist_ok=True)
    (d/'primary-header.bin').write_bytes(ph); (d/'primary-entries.bin').write_bytes(pe)
    (d/'backup-entries.bin').write_bytes(be); (d/'backup-header.bin').write_bytes(bh)
    (d/'manifest.json').write_text(json.dumps(m,indent=2)+'\n')

def main():
    ap=argparse.ArgumentParser(); ap.add_argument('--profile',type=Path,required=True); ap.add_argument('--backup-dir',type=Path,required=True); ap.add_argument('--out',type=Path,required=True); a=ap.parse_args()
    p=json.loads(a.profile.read_text()); ss=p['sector_size']; disk=p['disk_lbas']; g=p['gpt']
    first=(a.backup_dir/'sdc-first-4MiB.bin').read_bytes(); tail=(a.backup_dir/'sdc-last-4MiB.bin').read_bytes()
    ph=header(first,ss,ss); base=disk-len(tail)//ss; bh=find_backup(tail,base,ss)
    expected=[(ph['cur'],g['primary_header_lba']),(ph['ent'],g['primary_entries_lba']),(bh['ent'],g['backup_entries_lba']),(bh['cur'],g['backup_header_lba']),(ph['n'],g['num_entries']),(ph['esz'],g['entry_size'])]
    if any(x!=y for x,y in expected): raise SystemExit('REFUSING: GPT geometry differs from validated rodin profile')
    if ph['bak']!=bh['cur'] or bh['bak']!=ph['cur'] or ph['guid']!=bh['guid']: raise SystemExit('Primary/backup GPT mismatch')
    pt,plen,psecs=entries(first,ph,0,ss); bt,blen,bsecs=entries(tail,bh,base,ss)
    if bytes(pt[:plen])!=bytes(bt[:blen]): raise SystemExit('Primary/backup entry arrays differ')
    validate_stock(pt,p); validate_contiguous(layout(p,'stock')); validate_contiguous(layout(p,'development'))
    stock=dict(layout='stock-11g',sector_size=ss,disk_lbas=disk,super_bytes=p['stock']['super_bytes'],userdata_bytes=p['stock']['userdata_bytes'],primary_header_lba=ph['cur'],primary_entries_lba=ph['ent'],primary_entries_sectors=psecs,backup_entries_lba=bh['ent'],backup_entries_sectors=bsecs,backup_header_lba=bh['cur'],entries_crc32=f"0x{ph['ecrc']:08x}",partitions=layout(p,'stock'),source="exact original GPT structures from this device backup")
    write_set(a.out/'stock',bytes(first[ss:2*ss]),bytes(pt),bytes(bt),bytes(tail[bh['off']:bh['off']+ss]),stock)
    dpt,changes=patch_dev(bytearray(pt),p); dbt,_=patch_dev(bytearray(bt),p); dcrc=crc32(bytes(dpt[:plen]))
    if dcrc!=crc32(bytes(dbt[:blen])): raise SystemExit('Internal patched CRC mismatch')
    dev=dict(layout='development-20g',sector_size=ss,disk_lbas=disk,super_bytes=p['development']['super_bytes'],userdata_bytes=p['development']['userdata_bytes'],shift_lbas=p['development']['shift_lbas'],shift_bytes=p['development']['shift_bytes'],primary_header_lba=ph['cur'],primary_entries_lba=ph['ent'],primary_entries_sectors=psecs,backup_entries_lba=bh['ent'],backup_entries_sectors=bsecs,backup_header_lba=bh['cur'],entries_crc32=f"0x{dcrc:08x}",partitions=layout(p,'development'),changes=changes)
    write_set(a.out/'dev20g',patch_header(bytes(first[ss:2*ss]),ph,dcrc),bytes(dpt),bytes(dbt),patch_header(bytes(tail[bh['off']:bh['off']+ss]),bh,dcrc),dev)
    print('SOURCE GPT: VALID'); print(f"Stock super : {p['stock']['super_bytes']} bytes"); print(f"Dev super   : {p['development']['super_bytes']} bytes"); print(f"Shift       : {p['development']['shift_lbas']} x 4096 = {p['development']['shift_bytes']} bytes"); print(f"Dev GPT CRC : 0x{dcrc:08x}"); print(f"Output      : {a.out}"); print('NO BLOCK DEVICE WAS MODIFIED')
if __name__=='__main__': main()
