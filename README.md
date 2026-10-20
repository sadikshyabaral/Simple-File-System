# Simple File System

## Description

This project implements a simple file system utility that can read, list, get, and modify a filesystem image. The filesystem uses a superblock to store data, a FAT (File Allocation Table) to manage block allocation, and directory entries to store information about files and directories.

The project includes utilities for:
- listing files and directories stored inside a filesystem image
- retrieving files from the filesystem image
- adding files from the host operating system into the filesystem image

## Features Implemented

- Reading and parsing the filesystem superblock
- `diskinfo`: displays information about the filesystem image, including the superblock details, FAT usage, and the number of free blocks available
- Listing directory contents using `disklist`
- Searching for files and directories by path
- Extracting files from the filesystem image using `diskget`
- Copying files from the host system into the filesystem image using `diskput`

## Compilation

Compile using:
make

## Authors and acknowledgment
Sadikshya Baral 
