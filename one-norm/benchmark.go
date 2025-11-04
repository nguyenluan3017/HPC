package main

import (
	"flag"
	"fmt"
	"io"
	"log"
	"math"
	"os"
	"os/exec"
	"path/filepath"
	"runtime"
	"strconv"
	"strings"

	"gonum.org/v1/plot"
	"gonum.org/v1/plot/plotter"
	"gonum.org/v1/plot/plotutil"
	"gonum.org/v1/plot/vg"
	"gopkg.in/yaml.v3"
)

const (
	IMPL_THREADED = "threaded"
	IMPL_SERIAL   = "serial"

	SERIAL_RESULTS_FILE   = "serial.yaml"
	THREADED_RESULTS_FILE = "threaded.yaml"
)

type BenchmarkFlags struct {
	execPath       *string
	outputDir      *string
	resultsPath    *string
	imageDirPath   *string
	enablePlotting *bool
	verbose        *bool
	help           *bool
}

type CPUInfo struct {
	logicalCores uint
	l1dCache     uint
	l2Cache      uint
	l3Cache      uint
}

type TestConfiguration struct {
	blockSize          uint
	numberOfThreads    *uint
	numberOfIterations uint
	execPath           string
	outputFile         *os.File
	done               chan<- bool
}

type BenchmarkResults []BenchmarkResult

type BenchmarkResult struct {
	Metadata       Metadata        `yaml:"metadata"`
	Stats          Statistics      `yaml:"statistics"`
	IndividualRuns []IndividualRun `yaml:"individual_runs"`
}

type Metadata struct {
	Impl       string `yaml:"implementation"`
	MatrixSize uint   `yaml:"matrix_size"`
	BlockSize  uint   `yaml:"block_size"`
	NumThreads uint   `yaml:"num_threads"`
	NumRepeats uint   `yaml:"num_repeats"`
	Timestamp  uint64 `yaml:"timestamp"`
}

type Statistics struct {
	Multiplication  TimingStats `yaml:"multiplication"`
	NormComputation TimingStats `yaml:"norm_computation"`
	Total           TimingStats `yaml:"total"`
}

type TimingStats struct {
	AvgTime float64 `yaml:"average_time"`
	MinTime float64 `yaml:"min_time"`
	MaxTime float64 `yaml:"max_time"`
}

type IndividualRun struct {
	Run                int     `yaml:"run"`
	MultiplicationTime float64 `yaml:"multiplication_time"`
	NormTime           float64 `yaml:"norm_time"`
	TotalTime          float64 `yaml:"total_time"`
}

func (options *BenchmarkFlags) parse() {
	// Initialize the flag pointers
	options.execPath = flag.String("exec", "./bin/norm",
		"Path to executable to benchmark")

	options.outputDir = flag.String("output-dir", "./out",
		"Output directory for benchmark results")

	options.resultsPath = flag.String("results-path", "./results",
		"Path to existing benchmark results for plotting (used with -plot flag)")

	options.imageDirPath = flag.String("images-path", "./images",
		"Output directory for generated plot images")

	options.enablePlotting = flag.Bool("plot", false,
		"Enable plotting mode - creates charts from existing results instead of running benchmarks")

	options.verbose = flag.Bool("v", false,
		"Enable verbose output during benchmarking")

	options.help = flag.Bool("help", false,
		"Show this help message and exit")

	flag.Parse()

	if *options.help {
		flag.Usage()
		os.Exit(0)
	}
}

func parseSize(sizeStr, unitStr string) (uint, error) {
	size, err := strconv.ParseInt(sizeStr, 10, 64)
	if err != nil {
		return 0, err
	}

	switch {
	case strings.HasPrefix(unitStr, "K"):
		return uint(size * 1024), nil
	case strings.HasPrefix(unitStr, "M"):
		return uint(size * 1024 * 1024), nil
	case strings.HasPrefix(unitStr, "G"):
		return uint(size * 1024 * 1024 * 1024), nil
	case strings.HasPrefix(unitStr, "B"):
		return uint(size), nil
	default:
		return uint(size), nil
	}
}

func (info CPUInfo) getOptimalBlockSize() uint {
	if maxCacheSize := max(info.l3Cache, info.l2Cache, info.l1dCache); maxCacheSize > 0 {
		blockSizeSquared := float64(maxCacheSize) / float64(3*8)
		return uint(math.Sqrt(blockSizeSquared))
	}

	return 512
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
						size, err := strconv.ParseInt(parts[1], 10, 8*8)
						if err != nil {
							panic(err)
						}
						info.logicalCores = uint(size)
					} else if strings.Contains(line, "L1d cache:") {
						parts := strings.Fields(line)
						if len(parts) >= 4 {
							size, err := parseSize(parts[2], parts[3])
							if err != nil {
								panic(err)
							}
							info.l1dCache = size
						}
					} else if strings.Contains(line, "L2 cache:") {
						parts := strings.Fields(line)
						if len(parts) >= 4 {
							size, err := parseSize(parts[2], parts[3])
							if err != nil {
								panic(err)
							}
							info.l2Cache = size
						}
					} else if strings.Contains(line, "L3 cache:") {
						parts := strings.Fields(line)
						if len(parts) >= 4 {
							size, err := parseSize(parts[2], parts[3])
							if err != nil {
								panic(err)
							}
							info.l3Cache = size
						}
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

func runThreadedTest(testConfig TestConfiguration) {
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

func runSerialTest(testConfig TestConfiguration) {
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

func ensureOutputFile(path string) *os.File {
	if _, err := os.Stat(path); err == nil {
		os.Remove(path)
	}

	parent := filepath.Dir(path)
	os.MkdirAll(parent, 0755)

	f, err := os.OpenFile(path, os.O_CREATE|os.O_APPEND|os.O_WRONLY, 0644)
	if err != nil {
		panic(err)
	}
	return f
}

func benchmark(blockSize uint, numberOfThreads uint, numberOfIterations uint, execPath string, outputDir string, done chan<- bool) {
	threadedOutputFile := ensureOutputFile(filepath.Join(outputDir, THREADED_RESULTS_FILE))
	defer threadedOutputFile.Close()

	serialOutputFile := ensureOutputFile(filepath.Join(outputDir, SERIAL_RESULTS_FILE))
	defer serialOutputFile.Close()

	go runThreadedTest(TestConfiguration{
		blockSize:          blockSize,
		numberOfThreads:    &numberOfThreads,
		numberOfIterations: numberOfIterations,
		execPath:           execPath,
		done:               done,
		outputFile:         threadedOutputFile,
	})

	go runSerialTest(TestConfiguration{
		blockSize:          blockSize,
		numberOfIterations: numberOfIterations,
		execPath:           execPath,
		done:               done,
		outputFile:         serialOutputFile,
	})
}

func readBenchmarkResults(resultPath string) (BenchmarkResults, error) {
	file, err := os.Open(resultPath)
	if err != nil {
		return nil, err
	}
	defer file.Close()

	data, err := io.ReadAll(file)
	if err != nil {
		return nil, err
	}

	var results BenchmarkResults
	err = yaml.Unmarshal(data, &results)
	if err != nil {
		return nil, err
	}

	return results, nil
}

func plotRuntimeDependenceOnMatrixSize(serialResults BenchmarkResults, threadedResults BenchmarkResults, imageDirPath string) {
	p := plot.New()
	p.Title.Text = "Runtime vs Matrix Size"
	p.X.Label.Text = "Matrix Size"
	p.Y.Label.Text = "Average Execution Time (seconds)"

	// Convert serial results to plot points
	serialPts := make(plotter.XYs, len(serialResults))
	for i, result := range serialResults {
		serialPts[i].X = float64(result.Metadata.MatrixSize)
		serialPts[i].Y = result.Stats.Total.AvgTime
	}

	// Convert threaded results to plot points
	threadedPts := make(plotter.XYs, len(threadedResults))
	for i, result := range threadedResults {
		threadedPts[i].X = float64(result.Metadata.MatrixSize)
		threadedPts[i].Y = result.Stats.Total.AvgTime
	}

	// Add lines to plot
	err := plotutil.AddLinePoints(p,
		"Serial", serialPts,
		"Threaded", threadedPts)
	if err != nil {
		log.Fatal(err)
	}

	// Create the full path for the image file
	imagePath := filepath.Join(imageDirPath, "runtime_vs_matrix_size.png")

	// Save the plot
	if err := p.Save(12*vg.Inch, 8*vg.Inch, imagePath); err != nil {
		log.Fatal(err)
	}

	// Print markdown table with results
	fmt.Println("\n## Runtime vs Matrix Size Results")
	fmt.Println("| Matrix Size | Serial Time (s) | Threaded Time (s) | Speedup |")
	fmt.Println("|-------------|-----------------|-------------------|---------|")

	// Create maps for easy lookup
	serialTimes := make(map[uint]float64)
	threadedTimes := make(map[uint]float64)

	for _, result := range serialResults {
		serialTimes[result.Metadata.MatrixSize] = result.Stats.Total.AvgTime
	}

	for _, result := range threadedResults {
		threadedTimes[result.Metadata.MatrixSize] = result.Stats.Total.AvgTime
	}

	// Get all matrix sizes and sort them
	matrixSizes := make([]uint, 0)
	for size := range serialTimes {
		if _, exists := threadedTimes[size]; exists {
			matrixSizes = append(matrixSizes, size)
		}
	}

	// Sort matrix sizes
	for i := 0; i < len(matrixSizes)-1; i++ {
		for j := 0; j < len(matrixSizes)-i-1; j++ {
			if matrixSizes[j] > matrixSizes[j+1] {
				matrixSizes[j], matrixSizes[j+1] = matrixSizes[j+1], matrixSizes[j]
			}
		}
	}

	// Print table rows
	for _, size := range matrixSizes {
		serialTime := serialTimes[size]
		threadedTime := threadedTimes[size]
		speedup := serialTime / threadedTime

		fmt.Printf("| %-11d | %-15.6f | %-17.6f | %-7.2fx |\n",
			size, serialTime, threadedTime, speedup)
	}
	fmt.Println()
}

func plotSerialAndThreadedSpeedup(serialResults BenchmarkResults, threadedResults BenchmarkResults, imageDirPath string) {
	p := plot.New()
	p.Title.Text = "Runtime Comparison: Serial vs Threaded Implementation"
	p.Y.Label.Text = "Average Execution Time (seconds)"

	// Create maps for organizing data by matrix size
	serialTimes := make(map[uint]float64)
	threadedTimes := make(map[uint]float64)

	for _, result := range serialResults {
		serialTimes[result.Metadata.MatrixSize] = result.Stats.Total.AvgTime
	}

	for _, result := range threadedResults {
		threadedTimes[result.Metadata.MatrixSize] = result.Stats.Total.AvgTime
	}

	// Get all matrix sizes and sort them
	matrixSizes := make([]uint, 0)
	for size := range serialTimes {
		if _, exists := threadedTimes[size]; exists {
			matrixSizes = append(matrixSizes, size)
		}
	}

	// Sort matrix sizes for consistent ordering
	for i := 0; i < len(matrixSizes)-1; i++ {
		for j := 0; j < len(matrixSizes)-i-1; j++ {
			if matrixSizes[j] > matrixSizes[j+1] {
				matrixSizes[j], matrixSizes[j+1] = matrixSizes[j+1], matrixSizes[j]
			}
		}
	}

	// Prepare data for bar charts
	serialValues := make(plotter.Values, len(matrixSizes))
	threadedValues := make(plotter.Values, len(matrixSizes))
	labels := make([]string, len(matrixSizes))

	for i, size := range matrixSizes {
		serialValues[i] = serialTimes[size]
		threadedValues[i] = threadedTimes[size]
		labels[i] = fmt.Sprintf("%d", size)
	}

	// Create bar charts
	barWidth := vg.Points(25)

	serialBars, err := plotter.NewBarChart(serialValues, barWidth)
	if err != nil {
		log.Fatal(err)
	}
	serialBars.Color = plotutil.Color(0) // Red/Orange
	serialBars.Offset = -barWidth / 2

	threadedBars, err := plotter.NewBarChart(threadedValues, barWidth)
	if err != nil {
		log.Fatal(err)
	}
	threadedBars.Color = plotutil.Color(1) // Blue
	threadedBars.Offset = barWidth / 2

	// Add bars to plot
	p.Add(serialBars, threadedBars)

	// Set up X axis labels
	p.NominalX(labels...)

	// Add legend
	p.Legend.Add("Serial", serialBars)
	p.Legend.Add("Threaded", threadedBars)
	p.Legend.Top = true
	p.Legend.Left = true

	// Set Y axis to start from 0
	p.Y.Min = 0

	// Add X axis label
	p.X.Label.Text = "Matrix Size"

	// Create the full path for the image file
	imagePath := filepath.Join(imageDirPath, "runtime_comparison_bars.png")

	// Save the plot
	if err := p.Save(12*vg.Inch, 8*vg.Inch, imagePath); err != nil {
		log.Fatal(err)
	}

	// Print comparison information
	fmt.Println("\nRuntime Comparison Analysis:")
	fmt.Printf("%-12s | %-15s | %-15s | %-12s\n", "Matrix Size", "Serial (s)", "Threaded (s)", "Speedup")
	fmt.Printf("%-12s | %-15s | %-15s | %-12s\n", "---", "---", "---", "---")

	for _, size := range matrixSizes {
		serialTime := serialTimes[size]
		threadedTime := threadedTimes[size]
		speedup := serialTime / threadedTime

		fmt.Printf("%-12d | %-15.3f | %-15.3f | %-12.2fx\n",
			size, serialTime, threadedTime, speedup)
	}
}

func plotResults(serialResultsPath string, threadedResultsPath string, imageDirPath string) {
	serialResults, err := readBenchmarkResults(serialResultsPath)
	if err != nil {
		panic(err)
	}

	threadedResults, err := readBenchmarkResults(threadedResultsPath)
	if err != nil {
		panic(err)
	}

	// Ensure the image directory exists
	os.MkdirAll(imageDirPath, 0755)

	plotRuntimeDependenceOnMatrixSize(
		serialResults,
		threadedResults,
		imageDirPath,
	)

	plotSerialAndThreadedSpeedup(
		serialResults,
		threadedResults,
		imageDirPath,
	)
}

func main() {
	const numberOfIterations = uint(50)
	sysinfo := getCPUInfo()
	const blockSize = uint(512)
	done := make(chan bool)

	// Create and parse flags
	flags := &BenchmarkFlags{}
	flags.parse()

	if *flags.verbose {
		fmt.Printf("CPU Info: %d cores, L1d: %d KB, L2: %d KB, L3: %d KB\n",
			sysinfo.logicalCores, sysinfo.l1dCache/1024, sysinfo.l2Cache/1024, sysinfo.l3Cache/1024)
		fmt.Printf("Using executable: %s\n", *flags.execPath)
		fmt.Printf("Output directory: %s\n", *flags.outputDir)
		if *flags.enablePlotting {
			fmt.Printf("Image output directory: %s\n", *flags.imageDirPath)
		}
	}

	if *flags.enablePlotting {
		if *flags.verbose {
			fmt.Printf("Creating plots from results in: %s\n", *flags.resultsPath)
		}
		plotResults(
			filepath.Join(*flags.resultsPath, SERIAL_RESULTS_FILE),
			filepath.Join(*flags.resultsPath, THREADED_RESULTS_FILE),
			*flags.imageDirPath,
		)
	} else {
		if *flags.verbose {
			fmt.Println("Starting benchmark execution...")
		}

		benchmark(
			blockSize,
			sysinfo.logicalCores,
			numberOfIterations,
			*flags.execPath,
			*flags.outputDir,
			done,
		)

		<-done
		<-done

		if *flags.verbose {
			fmt.Println("Benchmark completed!")
		}
	}
}
