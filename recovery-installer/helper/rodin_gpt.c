typedef unsigned char u8;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef long long s64;
typedef unsigned long usize;

#define SYS_ioctl   29
#define SYS_openat  56
#define SYS_close   57
#define SYS_lseek   62
#define SYS_read    63
#define SYS_write   64
#define SYS_pread64 67
#define SYS_pwrite64 68
#define SYS_fsync   82
#define SYS_exit    93
#define AT_FDCWD    -100
#define O_RDONLY    0
#define O_RDWR      2
#define SEEK_SET    0
#define SEEK_END    2

#define SS 4096ULL
#define DISK_BYTES 264484421632ULL
#define DISK_LBAS  64571392ULL
#define GPT_ENTRIES 93
#define GPT_ENTRY_SIZE 128
#define GPT_EXACT_BYTES (GPT_ENTRIES * GPT_ENTRY_SIZE)
#define GPT_TABLE_BYTES (3 * 4096)
#define PRIMARY_HEADER_LBA 1ULL
#define PRIMARY_ENTRIES_LBA 2ULL
#define BACKUP_ENTRIES_LBA 64571384ULL
#define BACKUP_HEADER_LBA 64571391ULL

static long sc1(long n, long a) {
    register long x8 __asm__("x8") = n;
    register long x0 __asm__("x0") = a;
    __asm__ volatile("svc #0" : "+r"(x0) : "r"(x8) : "memory");
    return x0;
}
static long sc3(long n, long a, long b, long c) {
    register long x8 __asm__("x8") = n;
    register long x0 __asm__("x0") = a;
    register long x1 __asm__("x1") = b;
    register long x2 __asm__("x2") = c;
    __asm__ volatile("svc #0" : "+r"(x0) : "r"(x1), "r"(x2), "r"(x8) : "memory");
    return x0;
}
static long sc4(long n, long a, long b, long c, long d) {
    register long x8 __asm__("x8") = n;
    register long x0 __asm__("x0") = a;
    register long x1 __asm__("x1") = b;
    register long x2 __asm__("x2") = c;
    register long x3 __asm__("x3") = d;
    __asm__ volatile("svc #0" : "+r"(x0) : "r"(x1), "r"(x2), "r"(x3), "r"(x8) : "memory");
    return x0;
}

static long xopen(const char *p, int flags) { return sc4(SYS_openat, AT_FDCWD, (long)p, flags, 0); }
static long xclose(int fd) { return sc1(SYS_close, fd); }
static s64 xlseek(int fd, s64 off, int whence) { return (s64)sc3(SYS_lseek, fd, (long)off, whence); }
static long xwrite(int fd, const void *p, usize n) { return sc3(SYS_write, fd, (long)p, n); }
static long xpread(int fd, void *p, usize n, u64 off) { return sc4(SYS_pread64, fd, (long)p, n, (long)off); }
static long xpwrite(int fd, const void *p, usize n, u64 off) { return sc4(SYS_pwrite64, fd, (long)p, n, (long)off); }
static long xfsync(int fd) { return sc1(SYS_fsync, fd); }

static usize slen(const char *s) { usize n=0; while (s[n]) n++; return n; }
static int streq(const char *a, const char *b) { usize i=0; while (a[i] && b[i] && a[i]==b[i]) i++; return a[i]==0 && b[i]==0; }
static void mcpy(void *d_, const void *s_, usize n) { u8 *d=d_; const u8 *s=s_; for (usize i=0;i<n;i++) d[i]=s[i]; }
static int mcmp(const void *a_, const void *b_, usize n) { const u8 *a=a_, *b=b_; for (usize i=0;i<n;i++) if (a[i]!=b[i]) return (int)a[i]-(int)b[i]; return 0; }
static void mset(void *d_, u8 v, usize n) { u8 *d=d_; for (usize i=0;i<n;i++) d[i]=v; }
static void puts2(const char *s) { xwrite(1, s, slen(s)); }
static void putln(const char *s) { puts2(s); puts2("\n"); }
static void die(const char *s, int code) { puts2("ERROR: "); putln(s); sc1(SYS_exit, code); for(;;){} }

static u32 rd32(const u8 *p) { return (u32)p[0] | ((u32)p[1]<<8) | ((u32)p[2]<<16) | ((u32)p[3]<<24); }
static u64 rd64(const u8 *p) { u64 v=0; for (int i=7;i>=0;i--) v=(v<<8)|p[i]; return v; }
static void wr32(u8 *p, u32 v) { for(int i=0;i<4;i++){ p[i]=(u8)(v&0xff); v>>=8; } }
static void wr64(u8 *p, u64 v) { for(int i=0;i<8;i++){ p[i]=(u8)(v&0xff); v>>=8; } }

static u32 crc32(const u8 *p, usize n) {
    u32 c=0xffffffffU;
    for (usize i=0;i<n;i++) {
        c ^= p[i];
        for (int j=0;j<8;j++) c = (c>>1) ^ (0xedb88320U & (0U-(c&1U)));
    }
    return c ^ 0xffffffffU;
}

static int read_exact(int fd, void *buf_, usize n, u64 off) {
    u8 *buf=buf_; usize done=0;
    while (done<n) {
        long r=xpread(fd, buf+done, n-done, off+done);
        if (r<=0) return -1;
        done += (usize)r;
    }
    return 0;
}
static int write_exact(int fd, const void *buf_, usize n, u64 off) {
    const u8 *buf=buf_; usize done=0;
    while (done<n) {
        long r=xpwrite(fd, buf+done, n-done, off+done);
        if (r<=0) return -1;
        done += (usize)r;
    }
    return 0;
}

struct partdef { const char *name; int index; u64 stock_s, stock_e, dev_s, dev_e; u64 bytes; };
static const struct partdef parts[] = {
    {"super",83,524288,3407871,524288,5767167,0},
    {"ffu",84,3407872,3409919,5767168,5769215,8ULL*1024*1024},
    {"mem",85,3409920,3410943,5769216,5770239,4ULL*1024*1024},
    {"countrycode_a",86,3410944,3411199,5770240,5770495,1ULL*1024*1024},
    {"countrycode_b",87,3411200,3411455,5770496,5770751,1ULL*1024*1024},
    {"oops",88,3411456,3415551,5770752,5774847,16ULL*1024*1024},
    {"charger",89,3415552,3415807,5774848,5775103,1ULL*1024*1024},
    {"rescue",90,3415808,3448575,5775104,5807871,128ULL*1024*1024},
    {"blackbox",91,3448576,3490559,5807872,5849855,164ULL*1024*1024},
    {"userdata",92,3490560,64567287,5849856,64567287,0},
    {"flashinfo",93,64567288,64571383,64567288,64571383,0},
};
#define PART_COUNT ((int)(sizeof(parts)/sizeof(parts[0])))

static int name_eq(const u8 *e, const char *s) {
    const u8 *n=e+56; usize i=0;
    while (s[i]) {
        if (i>=36 || n[i*2]!=(u8)s[i] || n[i*2+1]!=0) return 0;
        i++;
    }
    if (i<36 && (n[i*2]!=0 || n[i*2+1]!=0)) return 0;
    return 1;
}

static int layout_of(const u8 *table) {
    int stock=1, dev=1;
    for (int i=0;i<PART_COUNT;i++) {
        const struct partdef *p=&parts[i];
        const u8 *e=table + (u64)(p->index-1)*GPT_ENTRY_SIZE;
        if (!name_eq(e,p->name)) return 0;
        u64 s=rd64(e+32), en=rd64(e+40);
        if (!(s==p->stock_s && en==p->stock_e)) stock=0;
        if (!(s==p->dev_s && en==p->dev_e)) dev=0;
    }
    if (stock) return 1;
    if (dev) return 2;
    return 0;
}

struct gptcopy {
    u8 header[SS];
    u8 table[GPT_TABLE_BYTES];
    int layout;
};

static void validate_header(const u8 *h, u64 expected_cur, u64 expected_bak, u64 expected_entries) {
    static const u8 sig[8]={'E','F','I',' ','P','A','R','T'};
    if (mcmp(h,sig,8)!=0) die("GPT signature mismatch",20);
    if (rd32(h+8)!=0x00010000U) die("GPT revision mismatch",21);
    u32 hsz=rd32(h+12); if (hsz<92 || hsz>SS) die("GPT header size invalid",22);
    if (rd64(h+24)!=expected_cur || rd64(h+32)!=expected_bak) die("GPT header LBA cross-reference mismatch",23);
    if (rd64(h+72)!=expected_entries) die("GPT entry-array LBA mismatch",24);
    if (rd32(h+80)!=GPT_ENTRIES || rd32(h+84)!=GPT_ENTRY_SIZE) die("GPT entry geometry mismatch",25);
    u8 tmp[SS]; mcpy(tmp,h,SS); u32 stored=rd32(tmp+16); wr32(tmp+16,0);
    if (crc32(tmp,hsz)!=stored) die("GPT header CRC mismatch",26);
}

static void load_copy(int fd, struct gptcopy *g, int backup) {
    u64 hlba=backup?BACKUP_HEADER_LBA:PRIMARY_HEADER_LBA;
    u64 elba=backup?BACKUP_ENTRIES_LBA:PRIMARY_ENTRIES_LBA;
    u64 bak=backup?PRIMARY_HEADER_LBA:BACKUP_HEADER_LBA;
    if (read_exact(fd,g->header,SS,hlba*SS)<0) die("Cannot read GPT header",27);
    validate_header(g->header,hlba,bak,elba);
    if (read_exact(fd,g->table,GPT_TABLE_BYTES,elba*SS)<0) die("Cannot read GPT entries",28);
    u32 stored=rd32(g->header+88);
    if (crc32(g->table,GPT_EXACT_BYTES)!=stored) die("GPT entry-array CRC mismatch",29);
    g->layout=layout_of(g->table);
    if (!g->layout) die("Unknown rodin GPT layout",30);
}

static int open_disk(void) {
    int fd=(int)xopen("/dev/block/sdc",O_RDWR);
    if (fd<0) die("Cannot open /dev/block/sdc",10);
    s64 sz=xlseek(fd,0,SEEK_END);
    if (sz!=(s64)DISK_BYTES) die("Unexpected /dev/block/sdc size",11);
    return fd;
}

static void patch_table(u8 *table, int target) {
    for(int i=0;i<PART_COUNT;i++) {
        const struct partdef *p=&parts[i];
        u8 *e=table+(u64)(p->index-1)*GPT_ENTRY_SIZE;
        u64 s=target==2?p->dev_s:p->stock_s;
        u64 en=target==2?p->dev_e:p->stock_e;
        wr64(e+32,s); wr64(e+40,en);
    }
}

static void patch_header(u8 *h, const u8 *table) {
    u32 hsz=rd32(h+12);
    wr32(h+88,crc32(table,GPT_EXACT_BYTES));
    wr32(h+16,0);
    wr32(h+16,crc32(h,hsz));
}

static void apply_target(int target) {
    int fd=open_disk();
    struct gptcopy p,b;
    load_copy(fd,&p,0); load_copy(fd,&b,1);
    if (mcmp(p.header+56,b.header+56,16)!=0) die("Primary/backup disk GUID mismatch",31);

    patch_table(b.table,target);
    patch_table(p.table,target);
    if (mcmp(p.table,b.table,GPT_EXACT_BYTES)!=0) die("Primary/backup GPT entries differ beyond geometry",32);
    patch_header(b.header,b.table);
    patch_header(p.header,p.table);

    if (write_exact(fd,b.table,GPT_TABLE_BYTES,BACKUP_ENTRIES_LBA*SS)<0) die("Cannot write backup GPT entries",40);
    if (write_exact(fd,b.header,SS,BACKUP_HEADER_LBA*SS)<0) die("Cannot write backup GPT header",41);
    if (xfsync(fd)<0) die("fsync failed after backup GPT",42);

    if (write_exact(fd,p.table,GPT_TABLE_BYTES,PRIMARY_ENTRIES_LBA*SS)<0) die("Cannot write primary GPT entries",43);
    if (write_exact(fd,p.header,SS,PRIMARY_HEADER_LBA*SS)<0) die("Cannot write primary GPT header",44);
    if (xfsync(fd)<0) die("fsync failed after primary GPT",45);

    struct gptcopy vp,vb;
    load_copy(fd,&vp,0); load_copy(fd,&vb,1);
    if (vp.layout!=target || vb.layout!=target) die("GPT readback layout verification failed",46);
    xclose(fd);
}

static u8 ioa[262144];
static u8 iob[262144];

static const char *path_for(const char *name, char *buf) {
    const char *prefix="/dev/block/by-name/";
    usize a=slen(prefix), b=slen(name);
    for(usize i=0;i<a;i++) buf[i]=prefix[i];
    for(usize i=0;i<b;i++) buf[a+i]=name[i];
    buf[a+b]=0; return buf;
}

static void copy_verify_one(int rawfd, const struct partdef *p, int target) {
    char path[96]; path_for(p->name,path);
    int sfd=(int)xopen(path,O_RDONLY);
    if (sfd<0) die("Cannot open source partition",50);
    s64 ssz=xlseek(sfd,0,SEEK_END);
    if (ssz!=(s64)p->bytes) die("Source partition size mismatch",51);
    u64 target_lba = target==2?p->dev_s:p->stock_s;
    u64 dst=target_lba*SS;
    u64 done=0;
    while(done<p->bytes) {
        usize chunk=(usize)((p->bytes-done)>sizeof(ioa)?sizeof(ioa):(p->bytes-done));
        if(read_exact(sfd,ioa,chunk,done)<0) die("Source partition read failed",52);
        if(write_exact(rawfd,ioa,chunk,dst+done)<0) die("Destination partition write failed",53);
        done+=chunk;
    }
    if(xfsync(rawfd)<0) die("fsync failed while staging partition",54);

    done=0;
    while(done<p->bytes) {
        usize chunk=(usize)((p->bytes-done)>sizeof(ioa)?sizeof(ioa):(p->bytes-done));
        if(read_exact(sfd,ioa,chunk,done)<0) die("Source reread failed",55);
        if(read_exact(rawfd,iob,chunk,dst+done)<0) die("Destination readback failed",56);
        if(mcmp(ioa,iob,chunk)!=0) die("Partition readback verification mismatch",57);
        done+=chunk;
    }
    xclose(sfd);
}

static void stage_target(int target) {
    int rawfd=open_disk();
    struct gptcopy p,b; load_copy(rawfd,&p,0); load_copy(rawfd,&b,1);
    /* Either known layout is accepted, including a recoverable mixed primary/backup state. */
    for(int i=1;i<=8;i++) copy_verify_one(rawfd,&parts[i],target);
    xclose(rawfd);
}

static void status_cmd(void) {
    int fd=open_disk(); struct gptcopy p,b; load_copy(fd,&p,0); load_copy(fd,&b,1); xclose(fd);
    if (p.layout==1 && b.layout==1) putln("stock");
    else if (p.layout==2 && b.layout==2) putln("dev20g");
    else putln("mixed");
}

static int main2(int argc, char **argv) {
    if(argc!=2) {
        putln("usage: rodin-gpt <status|stage-expand|apply-expand|stage-restore|apply-restore>");
        return 2;
    }
    if(streq(argv[1],"status")) { status_cmd(); return 0; }
    if(streq(argv[1],"stage-expand")) { stage_target(2); putln("stage-expand: OK"); return 0; }
    if(streq(argv[1],"apply-expand")) { apply_target(2); putln("apply-expand: OK"); return 0; }
    if(streq(argv[1],"stage-restore")) { stage_target(1); putln("stage-restore: OK"); return 0; }
    if(streq(argv[1],"apply-restore")) { apply_target(1); putln("apply-restore: OK"); return 0; }
    putln("unknown command"); return 2;
}

void c_start(u64 *sp) {
    int argc=(int)sp[0];
    char **argv=(char**)&sp[1];
    int rc=main2(argc,argv);
    sc1(SYS_exit,rc);
    for(;;){}
}
