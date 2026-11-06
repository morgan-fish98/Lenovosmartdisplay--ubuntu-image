#!/usr/bin/env python3
"""Patch files inside a small ext2/3/4 image in place (no mount, no e2fsprogs).

Only supports the classic ext2 block map (no extents), which is what the amber
boot partition uses. The replacement must fit in the blocks already allocated
to the file; this is enough to swap a DTB or an extlinux.conf of similar size.

Usage:
    python3 ext2patch.py IMAGE /path/inside/image newfile
"""
import struct
import sys


class Ext2:
    def __init__(self, path):
        self.f = open(path, "r+b")
        self.f.seek(1024)
        sb = self.f.read(1024)
        self.inodes_count, = struct.unpack_from("<I", sb, 0)
        self.blocks_count, = struct.unpack_from("<I", sb, 4)
        self.first_data_block, = struct.unpack_from("<I", sb, 20)
        self.log_block_size, = struct.unpack_from("<I", sb, 24)
        self.blocks_per_group, = struct.unpack_from("<I", sb, 32)
        self.inodes_per_group, = struct.unpack_from("<I", sb, 40)
        self.inode_size, = struct.unpack_from("<H", sb, 88)
        if self.inode_size == 0:
            self.inode_size = 128
        self.block_size = 1024 << self.log_block_size
        self.gdt_block = self.first_data_block + 1

    def close(self):
        self.f.close()

    def _read_block(self, blk):
        self.f.seek(blk * self.block_size)
        return self.f.read(self.block_size)

    def _write_block(self, blk, data):
        assert len(data) == self.block_size
        self.f.seek(blk * self.block_size)
        self.f.write(data)

    def inode_offset(self, ino):
        group = (ino - 1) // self.inodes_per_group
        index = (ino - 1) % self.inodes_per_group
        self.f.seek(self.gdt_block * self.block_size + group * 32 + 8)
        table_blk, = struct.unpack("<I", self.f.read(4))
        return table_blk * self.block_size + index * self.inode_size

    def inode(self, ino):
        off = self.inode_offset(ino)
        self.f.seek(off)
        raw = self.f.read(self.inode_size)
        mode, = struct.unpack_from("<H", raw, 0)
        size, = struct.unpack_from("<I", raw, 4)
        blocks, = struct.unpack_from("<I", raw, 28)
        ptrs = struct.unpack_from("<15I", raw, 40)
        return off, mode, size, blocks, ptrs

    def block_list(self, ptrs, size):
        """Return the data blocks of a file, in order, up to size bytes."""
        need = (size + self.block_size - 1) // self.block_size
        out = list(ptrs[:12])
        if len(out) >= need:
            return out[:need]
        if ptrs[12]:
            ind = struct.unpack("<%dI" % (self.block_size // 4),
                                self._read_block(ptrs[12]))
            out.extend(ind)
        if len(out) >= need:
            return out[:need]
        if ptrs[13]:
            dind = struct.unpack("<%dI" % (self.block_size // 4),
                                 self._read_block(ptrs[13]))
            for b in dind:
                if not b:
                    continue
                out.extend(struct.unpack("<%dI" % (self.block_size // 4),
                                         self._read_block(b)))
                if len(out) >= need:
                    break
        return out[:need]

    def read_file(self, ino):
        _, mode, size, _, ptrs = self.inode(ino)
        data = b"".join(self._read_block(b) for b in self.block_list(ptrs, size))
        return data[:size]

    def lookup(self, dir_ino, name):
        data = self.read_file(dir_ino)
        pos = 0
        while pos + 8 <= len(data):
            ino, rec_len, name_len, _ = struct.unpack_from("<IHBB", data, pos)
            if rec_len == 0:
                break
            if ino and data[pos + 8:pos + 8 + name_len] == name:
                return ino
            pos += rec_len
        raise FileNotFoundError(name.decode())

    def walk(self, path):
        ino = 2
        for part in [p for p in path.split("/") if p]:
            ino = self.lookup(ino, part.encode())
        return ino

    def replace(self, path, new_data):
        ino = self.walk(path)
        off, mode, size, blocks, ptrs = self.inode(ino)
        blks = self.block_list(ptrs, size)
        capacity = len(blks) * self.block_size
        if len(new_data) > capacity:
            raise SystemExit(
                f"{path}: new data {len(new_data)} B > allocated {capacity} B")
        # write blocks (zero-padded last one)
        data = new_data + b"\x00" * (capacity - len(new_data))
        for i, b in enumerate(blks):
            self._write_block(b, data[i * self.block_size:(i + 1) * self.block_size])
        # update i_size
        self.f.seek(off + 4)
        self.f.write(struct.pack("<I", len(new_data)))
        self.f.flush()
        return len(new_data), capacity


def main():
    if len(sys.argv) != 4:
        print(__doc__)
        return 1
    image, target, newfile = sys.argv[1:]
    new = open(newfile, "rb").read()
    fs = Ext2(image)
    try:
        n, cap = fs.replace(target, new)
        print(f"patched {target}: {n} B (capacity {cap} B)")
    finally:
        fs.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
