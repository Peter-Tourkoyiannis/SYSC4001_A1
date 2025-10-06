/**
 *
 * @file interrupts.cpp
 * @authors Sasisekhar Govind
 *          Peter Tourkoyiannis
 *          Akshwin Sellathurai
 *
 */

#include <interrupts.hpp>
#include <vector>
#include <algorithm>

int main(int argc, char** argv) {

    // vectors is a C++ std::vector of strings that contain the address of the ISR
    // delays  is a C++ std::vector of ints that contain the delays of each device
    // the index of these elements is the device number, starting from 0
    auto [vectors, delays] = parse_args(argc, argv);
    std::ifstream input_file(argv[1]);

    std::string trace;      //!< string to store single line of trace file
    std::string execution;  //!< string to accumulate the execution output

    /******************SIMULATION VARIABLES*************************/
    int current_time = 0;
    int context_save_time = 10;
    const int isr_activity_time = 40;
    const int iret_time = 1;

    // pending I/O
    std::vector<std::pair<int,int>> io_pending;
    /******************************************************************/

    // parse each line of the input trace file
    while (std::getline(input_file, trace)) {
        auto [activity, duration_intr] = parse_trace(trace);

        // for CPU
        if (activity == "CPU") {
            execution += std::to_string(current_time) + ", " + std::to_string(duration_intr) + ", CPU burst\n";
            current_time += duration_intr;
        }
        // SYSCALL
        else if (activity == "SYSCALL") {
            int intr_num = duration_intr; // device num

            // run boilerplate
            auto [log, new_time] = intr_boilerplate(current_time, intr_num, context_save_time, vectors);
            execution += log;
            current_time = new_time;

            // I/O
            int device_delay = 0;
            if (intr_num >= 0 && intr_num < (int)delays.size()) {
                device_delay = delays.at(intr_num);
            } else {
                // I assumed 0 again
                device_delay = 0;
            }

            int completion_time = current_time + device_delay;
            io_pending.push_back({intr_num, completion_time});

            // I/O request
            execution += std::to_string(current_time) + ", " + std::to_string(device_delay) + ", I/O request started for device " + std::to_string(intr_num) + "\n";
        }
        // END I/O
        else if (activity == "END_IO") {
            int intr_num = duration_intr;

            // find I/O entry for completion time
            int completion_time = -1;
            for (auto it = io_pending.begin(); it != io_pending.end(); ++it) {
                if (it->first == intr_num) {
                    completion_time = it->second;
                    io_pending.erase(it);
                    break;
                }
            }

            // advance timer if time exists
            if (completion_time != -1) {
                if (current_time < completion_time) {
                    current_time = completion_time;
                }
            } // no time exists

            // boilerplate
            auto [log, new_time] = intr_boilerplate(current_time, intr_num, context_save_time, vectors);
            execution += log;
            current_time = new_time;

            // do ISR
            int device_delay = 0;
            if (intr_num >= 0 && intr_num < (int)delays.size()) {
                device_delay = delays.at(intr_num);
            }

            // if device_delay is zero, let ISR run.
            int remaining = device_delay;
            int chunk_index = 0;
            while (remaining > 0) {
                int step = isr_activity_time;
                std::ostringstream msg;
                msg << "ISR activity from device " << intr_num << " chunk number " << chunk_index;
                execution += std::to_string(current_time) + ", " + std::to_string(step) + ", " + msg.str() + "\n";
                current_time += step;
                remaining -= step;
                ++chunk_index;
            }

            execution += std::to_string(current_time) + ", " + std::to_string(iret_time) + ", IRET\n";
            current_time += iret_time;
        }
    }

    input_file.close();
    write_output(execution);

    return 0;
}
