# Final score: 14.0/15.0

## Basic Functionality (1.0/1.0)

- [0.5/0.5] README present — README.md
- [0.5/0.5] `make` builds all four executables

## `diskinfo` (1.5/2.0)

- [1/1] `diskinfo` output format
- [0.5/0.5] `diskinfo` correct on `test.img`
- [0/0.5] `diskinfo` correct on `large.img` — one or more reported values are wrong

## `disklist` (4.0/4.0)

- [1/1] `disklist` produces a directory listing
- [1/1] `disklist non-empty.img /`
- [1/1] `disklist non-empty.img /sub_Dir`
- [1/1] `disklist large.img /dir1/dir2/dir3/dir4/dir5`

## `diskget` (4.0/4.0)

- [0.5/0.5] `diskget` reports a missing file
- [1/1] `diskget` copies `/sub_Dir/201.jpg`
- [1/1] `diskget` copies `/dir1/dir2/dir3/dir4/dir5/video.mp4`
- [1.5/1.5] copied files are byte-identical — 2/2 files matched the expected SHA-1

## `diskput` (3.5/4.0)

- [0.5/0.5] `diskput` reports a missing source file
- [2/2] `diskput` copies a file into the specified directory — 2/2 cases passed (root + text, existing sub-directory + binary)
- [1/1] copied-back file is identical to the original — 2/2 files read back byte-identical
- [0/0.5] `diskput` creates a missing sub-directory — `/new_dir` was not created in the image

## Output of failing cases

### `./diskinfo large.img`

Expected:

```text
Super block information:
Block size: 1024
Block count: 10240
FAT starts: 1
FAT blocks: 50
Root directory start: 51
Root directory blocks: 8

FAT information:
Free Blocks: 2369
Reserved Blocks: 49
Allocated Blocks: 7822
```

Your output:

```text
Super block information:
Block size: 1024
Block count: 10240
FAT starts: 1
FAT blocks: 50
Root directory start: 51
Root directory blocks: 8

FAT information:
Free Blocks: 4929
Reserved Blocks: 49
Allocated Blocks: 7822
```
