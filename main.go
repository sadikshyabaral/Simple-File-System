package main

import (
	"bytes"
	"encoding/binary"
	"fmt"
	"os"
	"time"
)

func getBigEndianBytes(input []byte) []byte {
	buf := new(bytes.Buffer)
	err := binary.Write(buf, binary.BigEndian, input)
	if err != nil {
		fmt.Println(err)
		return nil
	}
	return buf.Bytes()
}

func createRootStructure() []byte {
	bufStatus := make([]byte, 1)
	bufStatus[0] = 0x5 // 0000 0101

	bufStartingBlock := make([]byte, 4)
	binary.BigEndian.PutUint32(bufStartingBlock, uint32(51))

	bufNoBlocks := make([]byte, 4)
	binary.BigEndian.PutUint32(bufNoBlocks, uint32(8))

	bufFileSize := make([]byte, 4)
	binary.BigEndian.PutUint32(bufFileSize, uint32(0))

	t := time.Now()
	year := t.Year()
	month := t.Month()
	day := t.Day()
	hour := t.Hour()
	minute := t.Minute()
	second := t.Second()

	bufYear := make([]byte, 2)
	binary.BigEndian.PutUint16(bufYear, uint16(year))
	bufMonth := make([]byte, 1)
	bufMonth[0] = uint8(month)
	bufDay := make([]byte, 1)
	bufDay[0] = uint8(day)
	bufHour := make([]byte, 1)
	bufHour[0] = uint8(hour)
	bufMinute := make([]byte, 1)
	bufMinute[0] = uint8(minute)
	bufSecond := make([]byte, 1)
	bufSecond[0] = uint8(second)

	bufCreationTime := append(append(append(append(append(bufYear, bufMonth...), bufDay...), bufHour...), bufMinute...), bufSecond...)
	bufModifyTime := bufCreationTime

	bufFileName := make([]byte, 31)
	copy(bufFileName, getBigEndianBytes([]byte(".")))

	bufUnused := make([]byte, 6)
	for i := 0; i < 6; i++ {
		bufUnused[i] = 0xff
	}

	return append(append(append(append(append(append(append(bufStatus, bufStartingBlock...), bufNoBlocks...), bufFileSize...), bufCreationTime...), bufModifyTime...), bufFileName...), bufUnused...)
}

func main() {
	var blockSize uint16 = 512
	var blockCount uint16 = 6400
	var fileSize int64 = int64(blockSize) * int64(blockCount)

	file := "test.img"
	f, err := os.OpenFile(file, os.O_CREATE|os.O_WRONLY, 0644)
	if err != nil {
		fmt.Println(err)
		return
	}
	defer f.Close()

	zeros := make([]byte, fileSize)
	_, err = f.Write(zeros)
	if err != nil {
		fmt.Println(err)
		return
	}

	_, err = f.WriteAt(getBigEndianBytes([]byte("CSC360FS")), 0)
	if err != nil {
		fmt.Println(err)
		return
	}

	noFATBlocks := 50
	bufBlockSize := make([]byte, 2)
	bufBlockCount := make([]byte, 4)
	bufFATBlockStart := make([]byte, 4)
	bufNoFATBlocks := make([]byte, 4)
	bufRootBlockStarts := make([]byte, 4)
	bufNoRootBlocks := make([]byte, 4)
	binary.BigEndian.PutUint16(bufBlockSize, uint16(blockSize))
	binary.BigEndian.PutUint32(bufBlockCount, uint32(blockCount))
	binary.BigEndian.PutUint32(bufFATBlockStart, uint32(1))
	binary.BigEndian.PutUint32(bufNoFATBlocks, uint32(noFATBlocks))
	binary.BigEndian.PutUint32(bufRootBlockStarts, uint32(1+noFATBlocks))
	binary.BigEndian.PutUint32(bufNoRootBlocks, uint32(8))

	_, err = f.WriteAt(bufBlockSize, 8)
	if err != nil {
		fmt.Println(err)
		return
	}

	_, err = f.WriteAt(bufBlockCount, 10)
	if err != nil {
		fmt.Println(err)
		return
	}

	_, err = f.WriteAt(bufFATBlockStart, 14)
	if err != nil {
		fmt.Println(err)
		return
	}

	_, err = f.WriteAt(bufNoFATBlocks, 18)
	if err != nil {
		fmt.Println(err)
		return
	}

	_, err = f.WriteAt(bufRootBlockStarts, 22)
	if err != nil {
		fmt.Println(err)
		return
	}

	_, err = f.WriteAt(bufNoRootBlocks, 26)
	if err != nil {
		fmt.Println(err)
		return
	}

	bufBlockReserved := make([]byte, 4)
	binary.BigEndian.PutUint32(bufBlockReserved, uint32(1))

	for i := 0; i < noFATBlocks; i++ {
		offset := int64(512 + i*4)
		_, err := f.WriteAt(bufBlockReserved, offset)
		if err != nil {
			fmt.Println(err)
			return
		}
	}

	data := []byte{0xFF, 0xFF, 0xFF, 0xFF}
	offset := int64(int(blockSize) + noFATBlocks*4)
	_, err = f.WriteAt(data, offset)
	if err != nil {
		fmt.Println(err)
		return
	}

	data = []byte{0x00, 0x00, 0x00, 0x00}
	for i := 0; i <= 7; i++ {
		binary.BigEndian.PutUint32(data, uint32(52+i))
		offset := int64(int(blockSize) + noFATBlocks*4 + 4*i + 4)
		_, err = f.WriteAt(data, offset)
		if err != nil {
			fmt.Println(err)
			return
		}
	}

	data = []byte{0xFF, 0xFF, 0xFF, 0xFF}
	offset = int64(int(blockSize) + noFATBlocks*4 + 32)
	_, err = f.WriteAt(data, offset)
	if err != nil {
		fmt.Println(err)
		return
	}

	rootStructure := createRootStructure()
	_, err = f.WriteAt(rootStructure, int64(int(blockSize)*(noFATBlocks+1)))
	if err != nil {
		fmt.Println(err)
		return
	}

	bufRootA := make([]byte, 4)
	binary.BigEndian.PutUint32(bufRootA, uint32(51))

	_, err = f.WriteAt(bufRootA, int64(512))
	if err != nil {
		fmt.Println(err)
		return
	}
}
