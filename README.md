# Video-Capture-Server-Performance-Simulation
This project simulates the performance of a video capture server, focusing on the processing capabilities of the encoder and storage server. The simulation aims to evaluate the frame loss ratio and storage server utilization under different conditions.

## License

Introduction
The Video Capture Server Performance Simulation project is designed to evaluate the performance of a video capture server. The simulation models the arrival of video frames, encoding, and storage processes. It calculates the frame loss ratio and storage server utilization to help understand the system's performance under various conditions.

## Features
- Simulates the arrival, encoding, and storage of video frames

- Calculates frame loss ratio

- Calculates storage server utilization

## Installation
1. Clone the repository:
```bash
git clone https://github.com/Xiiii1/Video-Capture-Server-Performance-Simulation.git
```

2. Navigate to the project directory:   
```bash
cd Video-Capture-Server-Performance-Simulation
```

3. Compile the code using a C compiler (e.g., gcc):
```bash
gcc -o simulation simulation.c -lm
```

## Usage
1. Prepare the input file `simulation.in` with the following format:
```
<simulation_duration> <mean_interarrival_time> <mean_complexity>
```
For example:
```
8.0 0.00833 200.0
```
2. Run the simulation:
```bash
./simulation
```

3. Check the output file `simulation.out` for the results.

## Simulation Parameters
- `BUFFER_LIMIT`: The maximum capacity of the encoder buffer (default: 20)

- `C_ENC`: The processing capability of the encoder (default: 15800.0 fobs/sec)

- `C_STORAGE`: The processing capability of the storage server (default: 1600.0 bytes/sec)

- `mean_interarrival_time`: The average interarrival time of video frames

- `mean_complexity`: The average complexity of video frames

## Output
The output file simulation.out contains the following information:
- Buffer Limit
- Capacity of encoder
- Capacity of storage
- Frame Loss Ratio (f)
- Storage Utilization (u)

Example output:
```
Buffer Limit: 20
Capacity of encoder: 15800.00
Capacity of storage: 1600.00
Frame Loss Ratio (f): 0.12345678
Storage Utilization (u): 0.98765432
```
## Contributing
Contributions are welcome! Please open an issue or submit a pull request if you have any improvements or bug fixes.

## License
This project is licensed under the MIT License.
