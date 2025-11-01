package main

import (
	"flag"
	"fmt"
	"os"
	"os/exec"
	"runtime"
	"strconv"
	"strings"
)

const (
	IMPL_THREADED = "threaded"
	IMPL_SERIAL   = "serial"
)

type CPUInfo struct {
	logicalCores uint
	l1dCache     uint
	l2Cache      uint
	l3Cache      uint
}

type TestConfig struct {
    blockSize          uint
    numberOfThreads    *uint
    numberOfIterations uint
    execPath           string
	outputFile *os.File
	done               chan<- bool
}

func getCPUInfo() CPUInfo {
	var info CPUInfo

	switch runtime.GOOS {
	case "darwin":
		hw_keys := []string{"hw.physicalcpu", "hw.l1dcachesize", "hw.l2cachesize", "hw.l3cachesize"}

		for _, hw_key := range hw_keys {
			if cmd := exec.Command("sysctl", "-n", hw_key); cmd != nil {
				if output, err := cmd.Output(); err == nil {
					value, err := strconv.Atoi(strings.TrimSpace(string(output)))
					if err != nil {
						fmt.Printf("Warning: value of '%s' isn't an integer '%s'.\n", hw_key, string(output))
						continue
					}

					switch hw_key {
					case "hw.physicalcpu":
						info.logicalCores = uint(value)
					case "hw.l1dcachesize":
						info.l1dCache = uint(value)
					case "hw.l2cachesize":
						info.l2Cache = uint(value)
					case "hw.l3cachesize":
						info.l3Cache = uint(value)
					}
				}
			}
		}
	case "linux":
		if cmd := exec.Command("lscpu"); cmd != nil {
			if output, err := cmd.Output(); err == nil {
				lines := strings.Split(string(output), "\n")

				for _, line := range lines {
					if strings.Contains(line, "CPU(s):") && !strings.Contains(line, "NUMA") {
						parts := strings.Fields(line)
						fmt.Println(parts)
					} else if strings.Contains(line, "L1d cache:") {

					} else if strings.Contains(line, "L2 cache:") {

					} else if strings.Contains(line, "L3 cache:") {

					}
				}
			}
		}
	case "windows":
	default:
		panic(fmt.Sprintf("Unsupported platform: %s", runtime.GOOS))
	}

	return info
}

func runNorm(matrixSize uint, blockSize uint, numberOfThreads uint, numberOfIterations uint, impl string, execPath string, outputFile *os.File) {
	threadedCmd := exec.Command(
		execPath,
		"--matrix-size", fmt.Sprint(matrixSize),
		"--block-size", fmt.Sprint(blockSize),
		"--number-of-threads", fmt.Sprint(numberOfThreads),
		"--repeats", fmt.Sprint(numberOfIterations),
		"--impl", impl,
	)
	threadedCmd.Stderr = os.Stderr
	threadedCmd.Stdout = outputFile
	fmt.Println(threadedCmd)
	threadedCmd.Run()
}

func runThreadedTest(testConfig TestConfig) {
	for matrixSize := uint(1024); matrixSize <= 4096; matrixSize += 512 {
		runNorm(
			matrixSize,
			testConfig.blockSize,
			*testConfig.numberOfThreads,
			testConfig.numberOfIterations,
			IMPL_THREADED,
			testConfig.execPath,
			testConfig.outputFile,
		)
	}
	testConfig.done <- true
}

func runSerialTest(testConfig TestConfig) {
	for matrixSize := uint(1024); matrixSize <= 4096; matrixSize += 512 {
		runNorm(
			matrixSize,
			testConfig.blockSize,
			1,
			testConfig.numberOfIterations,
			IMPL_SERIAL,
			testConfig.execPath,
			testConfig.outputFile,
		)
	}
	testConfig.done <- true
}

func main() {
	const blockSize = 512
	const numberOfIterations = 1
	sysinfo := getCPUInfo()
	done := make(chan bool)	
	execPath := flag.String("exec", "./bin/norm", "Path to exectuable to benchmark")
	outputDir := flag.String("output-dir", "/tmp", "Output directory for benchmark results")
	numberOfThreads := sysinfo.logicalCores / 2	

	flag.Parse()

	threadedOutputFile, err := os.OpenFile(*outputDir + "/threaded_results.yaml", os.O_CREATE | os.O_APPEND | os.O_WRONLY, 0644)
	if err != nil {
		panic(err)
	}
	defer threadedOutputFile.Close()

	serialOutputFile, err := os.OpenFile(*outputDir + "/serial_results.yaml", os.O_CREATE | os.O_APPEND | os.O_WRONLY, 0644)
	if err != nil {
		panic(err)
	}
	defer serialOutputFile.Close()

	go runThreadedTest(TestConfig{
		blockSize: blockSize,
		numberOfThreads: &numberOfThreads,
		numberOfIterations: numberOfIterations,
		execPath: *execPath,
		done: done,
		outputFile: threadedOutputFile,
	})

	<- done	

	go runSerialTest(TestConfig{
		blockSize: blockSize,
		numberOfIterations: numberOfIterations,
		execPath: *execPath,
		done: done,
		outputFile: serialOutputFile,
	})

	<- done
}
